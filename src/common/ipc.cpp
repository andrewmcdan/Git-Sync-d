#include "ipc.h"

bool IPC::shutdown_trigger = false;

IPC::IPC()
{
    IPC::shutdown_trigger = false;
    this->startRunThread();
}

IPC::~IPC()
{
    std::unique_lock<std::mutex> lock(this->running_mutex);
    this->running = false;
    lock.unlock();
    this->ipcThread.join();
}

void IPC::startRunThread()
{
    std::unique_lock<std::mutex> lock(this->running_mutex);
    this->running = true;
    lock.unlock();
    this->ipcThread = std::thread([&]()
        { run(*this); });
}

bool IPC::pendingCommands()
{
    // check if there are any pending commands
    return this->commands_parsed.size() > 0;
}

bool IPC::shutdown()
{
    return shutdown_trigger;
}

void run(IPC& _this)
{
    // If the GUI is started as a child process of the service, we can use the managed_shared_memory
    // class to communicate between the two processes.
    // Otherwise we have to use a memory mapped file.
    // Normally, the GUI gets started by the service. But if the user exits the GUI, it will stop the service, and then is the
    // user starts the GUI again, the service gets started as a separate process instead of a child process.

    // Also, when using the CLI, we have to use a memory mapped file, because the CLI is started as a separate process.
    GIT_SYNC_D_MESSAGE::Error::error("IPC thread started", GIT_SYNC_D_MESSAGE::GENERIC_INFO);
    std::this_thread::sleep_for(std::chrono::milliseconds(1000));
    using namespace boost::asio;
    using namespace boost::system;
    io_service io_service;
    std::string pipe_name = "\\\\.\\pipe\\git-sync-d";
    error_code ec;
#if defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR* pSD;
    PSECURITY_DESCRIPTOR pSDDL;
    // Define the SDDL for the security descriptor
    // This SDDL string specifies that the pipe is open to Everyone
    // D: DACL, A: Allow, GA: Generic All, S-1-1-0: SID string for "Everyone"
    LPCSTR szSDDL = "D:(A;;GA;;;S-1-1-0)";
    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
        szSDDL, SDDL_REVISION_1, &pSDDL, NULL))
    {
        GIT_SYNC_D_MESSAGE::Error::error("Error converting string to security descriptor: " + std::to_string(GetLastError()), GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
    }
    pSD = (SECURITY_DESCRIPTOR*) pSDDL;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;
    // Create a named pipe.
    HANDLE pipe_handle = CreateNamedPipe(
        pipe_name.c_str(), // name of the pipe
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
        PIPE_UNLIMITED_INSTANCES, // unlimited instances of this pipe
        PIPE_BUFFER_SIZE,         // outbound buffer
        PIPE_BUFFER_SIZE,         // inbound buffer
        0,                        // use default wait time
        &sa                       // use default security attributes
    );
    if (pipe_handle == INVALID_HANDLE_VALUE)
    {
        GIT_SYNC_D_MESSAGE::Error::error("Error creating named pipe. error: " + std::to_string(GetLastError()), GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
        return;
    }
    GIT_SYNC_D_MESSAGE::Error::error("Named pipe created successfully", GIT_SYNC_D_MESSAGE::GENERIC_INFO);
    boost::asio::windows::stream_handle pipe(io_service, pipe_handle);
#elif defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
    local::stream_protocol::endpoint ep(pipe_name);
    local::stream_protocol::socket pipe(io_service);
    local::stream_protocol::acceptor acceptor(io_service, ep);
    acceptor.accept(pipe);
#endif
    std::thread pipeThread([&]()
        {
            while (true) {
                std::unique_lock<std::mutex> lock(_this.running_mutex);
                if (!_this.running) {
                    break;
                }
                lock.unlock();
                io_service.run(ec);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            }
        });

    size_t sleep_time = 50;
    std::vector<char> buffer_vect1(PIPE_BUFFER_SIZE);
    while (true)
    {
        std::unique_lock<std::mutex> lock(_this.running_mutex);
        if (!_this.running)
        {
            break;
        }
        lock.unlock();
        if (pipe.is_open())
        {
            pipe.async_read_some(buffer(buffer_vect1.data(), buffer_vect1.size()), [&](const error_code& ec, std::size_t bytes_transferred)
                {
                    // TODO: need to check if the pipe has ended and if so, close it and reopen it
                    if (ec) {
                        switch (ec.value()) {
                        case 536: // waiting for a connection
                        {
                            sleep_time = 500;
                            break;
                        }
                        case 109: // pipe ended. close and reopen
                        {
                            restartPipe(pipe, io_service, pipe_name);
                            break;
                        }
                        default:
                        {
                            GIT_SYNC_D_MESSAGE::Error::error("Error reading from named pipe: " + ec.message(), GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
                            GIT_SYNC_D_MESSAGE::Error::error("Error code: " + std::to_string(ec.value()), GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
                        }
                        }
                    } else {
                        sleep_time = 50;
                        std::string buffer(buffer_vect1.begin(), buffer_vect1.begin() + bytes_transferred);
                        // if (bytes_transferred > 0)
                            // GIT_SYNC_D_MESSAGE::Error::error("Read " + std::to_string(bytes_transferred) + " bytes from named pipe.\nBuffer size: " + std::to_string(buffer.size()) + "\nMessage: " + buffer, GIT_SYNC_D_MESSAGE::GENERIC_INFO);
                        // parse the buffer
                        // the buffer is in the format: startPattern, totalLength, commandLength, dataLenth, slot, command, data, endPattern
                        // startPattern is a string of 8 bytes: 0x11,0x22,0x33,0x44 + CRC32 of that string (33A8BD4E) = 0x11,0x22,0x33,0x44,0x33,0xA8,0xBD,0x4E
                        // endPattern is a string of 8 bytes: 0x88,0x77,0x66,0x55 + CRC32 of that string (F69C29D9) = 0x88,0x77,0x66,0x55,0xF6,0x9C,0x29,0xD9
                        // totalLength is a 4 byte integer (not including startPattern and endPattern)
                        // commandLength is a 4 byte integer (not including null terminator)
                        // dataLength is a 4 byte integer (not including null terminator)
                        // slot is a 4 byte integer
                        // command is a null terminated string
                        // data is a null terminated string

                        // union to convert from char array to int
                        union {
                            char c[4];
                            int i;
                        } slot;
                        union {
                            char c[4];
                            int i;
                        } commandLength;
                        union {
                            char c[4];
                            int i;
                        } dataLength;
                        union {
                            char c[4];
                            int i;
                        } totalLength;
                        // get the start pattern
                        std::string startPattern(buffer.begin(), buffer.begin() + 8);
                        if (startPattern != std::string("\x11\x22\x33\x44\x33\xA8\xBD\x4E"))
                        {
                            GIT_SYNC_D_MESSAGE::Error::error("Error reading from named pipe: start pattern not found", GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
                            return;
                        }
                        // get the total length
                        std::string totalLengthStr(buffer.begin() + 8, buffer.begin() + 12);
                        memcpy(totalLength.c, totalLengthStr.c_str(), 4);
                        // get the command length
                        std::string commandLengthStr(buffer.begin() + 12, buffer.begin() + 16);
                        memcpy(commandLength.c, commandLengthStr.c_str(), 4);
                        // get the data length
                        std::string dataLengthStr(buffer.begin() + 16, buffer.begin() + 20);
                        memcpy(dataLength.c, dataLengthStr.c_str(), 4);
                        // get the slot
                        std::string slotStr(buffer.begin() + 20, buffer.begin() + 24);
                        memcpy(slot.c, slotStr.c_str(), 4);
                        // get the command
                        std::string commandStr(buffer.begin() + 24, buffer.begin() + 24 + commandLength.i);
                        // get the data
                        std::string dataStr(buffer.begin() + 24 + commandLength.i, buffer.begin() + 24 + commandLength.i + dataLength.i);
                        // get the end pattern
                        std::string endPattern(buffer.begin() + 24 + commandLength.i + dataLength.i, buffer.begin() + 24 + commandLength.i + dataLength.i + 8);
                        if (endPattern != std::string("\x88\x77\x66\x55\xF6\x9C\x29\xD9"))
                        {
                            GIT_SYNC_D_MESSAGE::Error::error("Error reading from named pipe: end pattern not found", GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
                            return;
                        }

                        GIT_SYNC_D_MESSAGE::Error::error("Command: " + commandStr + "\nData: " + dataStr, GIT_SYNC_D_MESSAGE::GENERIC_INFO);

                        std::unique_lock<std::mutex> lock(_this.commands_vectors_mutex);
                        _this.commands_to_parse.push_back(std::make_pair(slot.i, commandStr));
                        _this.data_to_parse.push_back(std::make_pair(slot.i, dataStr));
                        lock.unlock();
                    }
                });
        } else
        {
            GIT_SYNC_D_MESSAGE::Error::error("Pipe is not open", GIT_SYNC_D_MESSAGE::GENERIC_INFO);
        }
        // sleep for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    }
    pipeThread.join();
    pipe.close();
}

#if defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
void restartPipe(boost::asio::windows::stream_handle& pipe, boost::asio::io_service& io_service, std::string pipe_name)
{
    pipe.close();
    SECURITY_ATTRIBUTES sa;
    SECURITY_DESCRIPTOR* pSD;
    PSECURITY_DESCRIPTOR pSDDL;
    // Define the SDDL for the security descriptor
    // This SDDL string specifies that the pipe is open to Everyone
    // D: DACL, A: Allow, GA: Generic All, S-1-1-0: SID string for "Everyone"
    LPCSTR szSDDL = "D:(A;;GA;;;S-1-1-0)";
    if (!ConvertStringSecurityDescriptorToSecurityDescriptor(
        szSDDL, SDDL_REVISION_1, &pSDDL, NULL))
    {
        GIT_SYNC_D_MESSAGE::Error::error("Error converting string to security descriptor: " + std::to_string(GetLastError()), GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
        return;
    }
    pSD = (SECURITY_DESCRIPTOR*) pSDDL;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;
    CloseHandle(pipe.native_handle());
    // Create a named pipe.
    HANDLE pipe_handle = CreateNamedPipe(
        pipe_name.c_str(), // name of the pipe
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
        PIPE_UNLIMITED_INSTANCES, // unlimited instances of this pipe
        PIPE_BUFFER_SIZE,         // outbound buffer
        PIPE_BUFFER_SIZE,         // inbound buffer
        0,                        // use default wait time
        &sa                       // use default security attributes
    );
    if (pipe_handle == INVALID_HANDLE_VALUE)
    {
        GIT_SYNC_D_MESSAGE::Error::error("Error creating named pipe. error: " + std::to_string(GetLastError()), GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
        return;
    }
    pipe = boost::asio::windows::stream_handle(io_service, pipe_handle);
}
#elif defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
void restartPipe(boost::asio::local::stream_protocol::socket& pipe, boost::asio::io_service& io_service, std::string pipe_name)
{
    // Create a local stream protocol socket.
    boost::asio::local::stream_protocol::endpoint ep(pipe_name);
    pipe(io_service);
    boost::asio::local::stream_protocol::acceptor acceptor(io_service, ep);
    acceptor.accept(pipe);
}
#endif

/**
 * Messages that may be received by the service/daemon:
 * - Trigger add file
 * - Trigger remove file
 * - Trigger add directory
 * - Trigger remove directory
 * - Trigger add repository
 * - Trigger remove repository
 * - Trigger add credential
 * - Trigger remove credential
 * - Trigger Sync
 * - Trigger Sync all
 *
 *
 */