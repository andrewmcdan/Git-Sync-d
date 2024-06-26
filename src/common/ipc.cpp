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
    this->ipcThread = std::thread([&]() { run(*this); });
}

bool IPC::pendingCommands()
{
    // check if there are any pending commands 
    return this->commands_parsed.size() > 0;
}

bool IPC::shutdown()
{
    std::unique_lock<std::mutex> lock(this->running_mutex);
    this->running = false;
    lock.unlock();
    IPC::shutdown_trigger = true;
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
    std::this_thread::sleep_for(std::chrono::milliseconds(100));
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
            szSDDL, SDDL_REVISION_1, &pSDDL, NULL)) {
        GIT_SYNC_D_MESSAGE::Error::error("Error converting string to security descriptor: " + std::to_string(GetLastError()), GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
    }
    pSD = (SECURITY_DESCRIPTOR*)pSDDL;
    sa.nLength = sizeof(SECURITY_ATTRIBUTES);
    sa.lpSecurityDescriptor = pSD;
    sa.bInheritHandle = FALSE;
    // Create a named pipe.
    HANDLE pipe_handle = CreateNamedPipe(
        pipe_name.c_str(), // name of the pipe
        PIPE_ACCESS_DUPLEX | FILE_FLAG_OVERLAPPED,
        PIPE_TYPE_BYTE | PIPE_READMODE_BYTE,
        PIPE_UNLIMITED_INSTANCES, // unlimited instances of this pipe
        PIPE_BUFFER_SIZE, // outbound buffer
        PIPE_BUFFER_SIZE, // inbound buffer
        0, // use default wait time
        &sa // use default security attributes
    );
    if (pipe_handle == INVALID_HANDLE_VALUE) {
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

    std::thread pipeThread([&]() {
            while (true) {
                std::unique_lock<std::mutex> lock(_this.running_mutex);
                if (!_this.running) {
                    break;
                }
                lock.unlock();
                io_service.run(ec);
                std::this_thread::sleep_for(std::chrono::milliseconds(10));
            } });
    std::thread respondingThread([&]() {
            while (true) {
                std::unique_lock<std::mutex> lock(_this.running_mutex);
                if (!_this.running) {
                    break;
                }
                lock.unlock();
                response response_to_send = std::make_pair(0, "");
                std::unique_lock<std::mutex> responses_vectors_mutex_lock(_this.responses_vectors_mutex);
                bool responses_to_send_empty = (_this.responses_to_send.size() == 0);
                if (!responses_to_send_empty)
                {
                    response_to_send = _this.responses_to_send[0];
                    _this.responses_to_send.erase(_this.responses_to_send.begin());
                }
                responses_vectors_mutex_lock.unlock();
                if (!responses_to_send_empty) {
                    union {
                        char c[sizeof(size_t)];
                        size_t i;
                    } responseSize;
                    responseSize.i = response_to_send.first;
                    std::string responseStr = std::string(responseSize.c, sizeof(size_t)) + response_to_send.second;
                    GIT_SYNC_D_MESSAGE::Error::error("Sending response: " + responseStr, GIT_SYNC_D_MESSAGE::GENERIC_INFO);
                    std::vector<char> buffer_vect(responseStr.begin(), responseStr.end());
                    buffer_vect.push_back('\0');
                    error_code ec;
                    pipe.write_some(buffer(buffer_vect.data(), buffer_vect.size()), ec);
                    if (ec) {
                        GIT_SYNC_D_MESSAGE::Error::error("Error writing to named pipe: " + ec.message(), GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
                        GIT_SYNC_D_MESSAGE::Error::error("Error code: " + std::to_string(ec.value()), GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
                    }
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } });
    std::thread commandParserThread = std::thread([&]() {
            while (true) {
                std::unique_lock<std::mutex> lock(_this.running_mutex);
                if (!_this.running) {
                    break;
                }
                lock.unlock();
                std::unique_lock<std::mutex> commands_data_vectors_mutex_lock(_this.commands_data_vectors_mutex);
                bool commands_to_parse_empty = (_this.commands_to_parse.size() == 0);
                commands_data_vectors_mutex_lock.unlock();
                if (!commands_to_parse_empty) {
                    parseCommands(_this.commands_to_parse, _this.data_to_parse, _this.responses_to_send, _this.commands_data_vectors_mutex, _this.responses_vectors_mutex);
                }
                std::this_thread::sleep_for(std::chrono::milliseconds(50));
            } });

    size_t sleep_time = 50;
    std::vector<char> buffer_vect1(PIPE_BUFFER_SIZE);
    while (true) {
        if (IPC::shutdown_trigger) {
            _this.shutdown();
        }
        std::unique_lock<std::mutex> running_mutex_lock(_this.running_mutex);
        if (!_this.running) {
            break;
        }
        running_mutex_lock.unlock();
        if (pipe.is_open()) {
            pipe.async_read_some(buffer(buffer_vect1.data(), buffer_vect1.size()), [&](const error_code& ec, std::size_t bytes_transferred) {
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
                        if (bytes_transferred > 0)
                            GIT_SYNC_D_MESSAGE::Error::error("Read " + std::to_string(bytes_transferred) + " bytes from named pipe.\nBuffer size: " + std::to_string(buffer.size()) + "\nMessage: " + buffer, GIT_SYNC_D_MESSAGE::GENERIC_INFO);
                        // parse the buffer
                        // the buffer is in the format: startPattern, totalLength, dataLength, slot, command, data, endPattern
                        // startPattern is a string of 8 bytes: START_PATTERN_STRING
                        // endPattern is a string of 8 bytes: END_PATTERN_STRING
                        // totalLength is a 4 byte integer (not including startPattern and endPattern)
                        // dataLength is a 4 byte integer (not including null terminator)
                        // slot is a 4 byte integer
                        // command is a null terminated string
                        // data is a null terminated string

                        // union to convert from char array to int

                        union {
                            char c[sizeof(unsigned int)];
                            unsigned int i;
                        } slot;
                        union {
                            char c[sizeof(unsigned int)];
                            unsigned int i;
                        } dataLength;
                        union {
                            char c[sizeof(unsigned int)];
                            unsigned int i;
                        } totalLength;
                        union {
                            char c[sizeof(unsigned int)];
                            COMMAND_CODE i;
                        } commandCode;

                        size_t bufferIndex = 0;
                        while (bufferIndex < buffer.size())
                        {
                            size_t startPatternSize = std::string(START_PATTERN_STRING).size();
                            if (bufferIndex + startPatternSize < buffer.size())
                            {
                                std::string startPattern = buffer.substr(bufferIndex, startPatternSize);
                                if (startPattern != std::string(START_PATTERN_STRING))
                                {
                                    bufferIndex++;
                                    continue;
                                }
                                bufferIndex += startPatternSize;
                            } else
                            {
                                break;
                            }
                            // get the total length
                            if (bufferIndex + sizeof(unsigned int) < buffer.size())
                            {
                                std::string totalLengthStr(buffer.begin() + bufferIndex, buffer.begin() + bufferIndex + sizeof(unsigned int));
                                memcpy(totalLength.c, totalLengthStr.c_str(), sizeof(unsigned int));
                                bufferIndex += sizeof(unsigned int);
                            } else
                            {
                                break;
                            }
                            // check to make sure that the buffer is at least as big as bufferIndex + totalLength - 8 - 4 (subtract 8 for startPattern and 4 for totalLength since bufferIndex was increased by 8 and 4 respectively)
                            if (bufferIndex + totalLength.i - startPatternSize - sizeof(unsigned int) > buffer.size())
                            {
                                break;
                            }
                            // get the data length
                            std::string dataLengthStr(buffer.begin() + bufferIndex, buffer.begin() + bufferIndex + sizeof(unsigned int));
                            memcpy(dataLength.c, dataLengthStr.c_str(), sizeof(unsigned int));
                            bufferIndex += sizeof(unsigned int);
                            // get the slot
                            std::string slotStr(buffer.begin() + bufferIndex, buffer.begin() + bufferIndex + sizeof(unsigned int));
                            memcpy(slot.c, slotStr.c_str(), sizeof(unsigned int));
                            bufferIndex += sizeof(unsigned int);
                            // get the command
                            std::string commandStr(buffer.begin() + bufferIndex, buffer.begin() + bufferIndex + sizeof(unsigned int));
                            memcpy(commandCode.c, commandStr.c_str(), sizeof(unsigned int));
                            bufferIndex += sizeof(unsigned int);
                            // get the data
                            std::string dataStr(buffer.begin() + bufferIndex, buffer.begin() + bufferIndex + dataLength.i);
                            bufferIndex += dataLength.i;
                            // get the end pattern
                            size_t endPatternSize = std::string(END_PATTERN_STRING).size();
                            std::string endPattern(buffer.begin() + bufferIndex, buffer.begin() + bufferIndex + endPatternSize);
                            if (endPattern != std::string(END_PATTERN_STRING))
                            {
                                break;
                            }
                            bufferIndex += 8;
                            std::unique_lock commands_data_vectors_mutex_lock(_this.commands_data_vectors_mutex);
                            _this.commands_to_parse.push_back(std::make_pair(slot.i, commandCode.i));
                            _this.data_to_parse.push_back(std::make_pair(slot.i, dataStr));
                            commands_data_vectors_mutex_lock.unlock();
                        }

                        std::unique_lock<std::mutex> lock(_this.commands_data_vectors_mutex);
                        for (auto& command : _this.commands_to_parse)
                        {
                            GIT_SYNC_D_MESSAGE::Error::error("Command code: " + std::to_string(command.second) + "\nslot: " + std::to_string(command.first) + "\n\n", GIT_SYNC_D_MESSAGE::GENERIC_INFO);
                        }
                        for (auto& data : _this.data_to_parse)
                        {
                            GIT_SYNC_D_MESSAGE::Error::error("Data: " + data.second + "\nslot: " + std::to_string(data.first) + "\n\n", GIT_SYNC_D_MESSAGE::GENERIC_INFO);
                        }
                        lock.unlock();
                    } });
        } else {
            GIT_SYNC_D_MESSAGE::Error::error("Pipe is not open", GIT_SYNC_D_MESSAGE::GENERIC_INFO);
        }

        // sleep for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(sleep_time));
    }
    pipeThread.join();
    respondingThread.join();
    pipe.close();
}

/**
 * @brief Parse the commands and data vectors
 *
 * @param commands_to_parse The commands to parse
 * @param data_to_parse The data to parse
 * @param responses_to_send The responses to send back to the GUI / CLI
 * @param commands_data_vectors_mutex The mutex for the commands and data vectors
 * @return true If the commands and data were parsed successfully
 * @return false If the commands and data were not parsed successfully
 */
bool parseCommands(
    std::vector<command>& commands_to_parse,
    std::vector<data>& data_to_parse,
    std::vector<response>& responses_to_send,
    std::mutex& commands_data_vectors_mutex,
    std::mutex& responses_vectors_mutex)
{
    bool allCommandsParsed = true;

    std::vector<command> commands_to_parse_local;
    std::vector<data> data_to_parse_local;
    std::vector<response> responses_to_send_local;

    std::unique_lock<std::mutex> commands_data_vectors_mutex_lock(commands_data_vectors_mutex);
    commands_to_parse_local = commands_to_parse;
    data_to_parse_local = data_to_parse;
    commands_to_parse.clear();
    data_to_parse.clear();
    commands_data_vectors_mutex_lock.unlock();

    if (commands_to_parse_local.size() == 0) {
        return true;
    }

    for (size_t i = 0; i < commands_to_parse_local.size(); i++) {
        // parse the command
        // the command is in the format: slot, command
        // command is a null terminated string
        // slot is a 4 byte integer
        // the command is one of the following:
        // - Trigger add file
        switch (commands_to_parse_local[i].second) {
        case COMMAND_ADD_FILE: {
            std::string data = "";
            // first check the corresponding index in data_to_parse_local to see if the slots match
            if (data_to_parse_local[i].first == commands_to_parse_local[i].first) {
                data = data_to_parse_local[i].second;
            } else {
                for (auto& data_pair : data_to_parse_local) {
                    if (data_pair.first == commands_to_parse_local[i].first) {
                        data = data_pair.second;
                        break;
                    }
                }
            }
            if(data == "") {
                GIT_SYNC_D_MESSAGE::Error::error("No data for addFile command.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                break;
            }
            // parse data
            // this will be in the format: key:value\nkey:value\nkey:value\n etc...
            // file: <full path to file> (c:/users/username/documents/file.txt)
            // directory: <relative path in the destination repository> (subdirectory1/subdirectory2)
            // destRepository: <name of the destination repository> (example-repository)
            // credential: <name of the credential> (example-credential) {optional} - if not specified, the default credential will be used
            // localRepository: <path to the local repository> (c:/users/username/documents/example-repository) {optional}
            // syncTypeString: <sync type> 4 byte int, result of OR'ing SYNC_TYPE_REPO, SYNC_TYPE_TIME_FRAME, SYNC_TYPE_DIRECTORY, SYNC_TYPE_FILE, etc...
            // syncTimeFrame: <time frame> (1h, 1d, 1w, 1m, 1y, 1h30m, 1d12h, etc...) {optional} - if not specified, the default time frame will be used

            std::string filePath = "";
            std::string destDirectoryPath = "";
            std::string destRepository = "";
            std::string credential = "";
            std::string localRepository = "";
            std::string syncTypeString = "";
            std::string syncTimeFrame = "";

            std::vector<std::string> dataLines;
            std::string dataLine;
            for (size_t i = 0; i < data.size(); i++) {
                if (data[i] == '\n') {
                    dataLines.push_back(dataLine);
                    dataLine = "";
                } else {
                    dataLine += data[i];
                }
            }
            if (dataLine != "") {
                dataLines.push_back(dataLine);
            }

            for (auto& line : dataLines) {
                std::string key;
                std::string value;
                bool keyFound = parseKeyValue(line, key, value);
                if (!keyFound) {
                    GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addFile command. Unable to parse key/value pair: " + line, GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                    allCommandsParsed = false;
                    continue;
                }
                if (key == "file")
                    filePath = value;
                else if (key == "directory")
                    destDirectoryPath = value;
                else if (key == "destRepository")
                    destRepository = value;
                else if (key == "credential")
                    credential = value;
                else if (key == "localRepository")
                    localRepository = value;
                else if (key == "syncType")
                    syncTypeString = value;
                else if (key == "syncTimeFrame")
                    syncTimeFrame = value;
            }
            if (!allCommandsParsed)
                continue;

            if (filePath == "") {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addFile command. Unable to parse source file path.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }
            if (destDirectoryPath == "") {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addFile command. Unable to parse destination directory path.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }
            if (destRepository == "") {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addFile command. Unable to parse destination repository name.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }
            if (syncTypeString == "") {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addFile command. Unable to parse sync type.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }

            // verify that the data is valid
            // - check that the file exists
            if (!std::filesystem::exists(filePath)) {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addFile command. Source file does not exist. filePath: " + filePath, GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }

            union {
                char c[4];
                unsigned int i;
            } syncType_un;
            memcpy(syncType_un.c, syncTypeString.c_str(), 4);

            if (syncType_un.i & SYNC_TYPE_REPO) {
                // check that the folder exists within a repo
                if (!withinRepo(filePath) && localRepository == "") {
                    GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addFile command. Source file is not within a repository.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                    allCommandsParsed = false;
                    continue;
                }
            }

            size_t syncTime_seconds = 0;
            if (syncType_un.i & SYNC_TYPE_TIME_FRAME) {
                // check that the time frame is valid
                if (!parseTimeFrame(syncTimeFrame, syncTime_seconds)) {
                    GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addFile command. Unable to parse time frame.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                    allCommandsParsed = false;
                    continue;
                }
            }

            // TODO: add file -- The following call should return an int that corresponds to something like: 0 = success, 1 = error, 2 = already exists, 3 = exists in synced folder, etc...
            // MainLogic::addFile(filePath, destDirectoryPath, destRepository, credential, localRepository, syncType_un.i, syncTime_seconds);
            std::string responseStr = "";
            int returnValue = 0; // = MainLogic::addFile(filePath, destDirectoryPath, destRepository, credential, localRepository, syncType_un.i, syncTime_seconds);
            responseStr = "code:" + std::to_string(returnValue) + ":" + std::to_string(commands_to_parse_local[i].second) + "-" + std::to_string(commands_to_parse_local[i].first);
            responses_to_send_local.push_back(std::make_pair(responseStr.size(), responseStr));
            break;
        }
        // - Trigger remove file
        case COMMAND_REMOVE_SYNC: // TODO: change to COMMAND_REMOVE_SYNC that can act on files or folders and verify that existing code works for both
        {
            std::string data = "";
            // first check the corresponding index in data_to_parse_local to see if the slots match
            if (data_to_parse_local[i].first == commands_to_parse_local[i].first) {
                data = data_to_parse_local[i].second;
            } else {
                for (auto& data_pair : data_to_parse_local) {
                    if (data_pair.first == commands_to_parse_local[i].first) {
                        data = data_pair.second;
                        break;
                    }
                }
            }
            if(data == "") {
                GIT_SYNC_D_MESSAGE::Error::error("No data for removeFile command.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                break;
            }
            // parse data
            // this will be in the format: key:value
            // file: <full path to file> (c:/users/username/documents/file.txt)
            std::string filePath = "";
            std::vector<std::string> dataLines;
            std::string dataLine;
            for (size_t i = 0; i < data.size(); i++) {
                if (data[i] == '\n') {
                    dataLines.push_back(dataLine);
                    dataLine = "";
                } else {
                    dataLine += data[i];
                }
            }
            if (dataLine != "") {
                dataLines.push_back(dataLine);
            }

            for (auto& line : dataLines) {
                std::string key;
                std::string value;
                bool keyFound = parseKeyValue(line, key, value);
                if (!keyFound) {
                    GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for removeFile command. Unable to parse key/value pair.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                    allCommandsParsed = false;
                    continue;
                }
                if (key == "file") {
                    filePath = value;
                }
            }
            if (!allCommandsParsed)
                continue;

            if (filePath == "") {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for removeFile command. Unable to parse source file path.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }

            // verify that the data is valid
            // - check that the file exists
            if (!std::filesystem::exists(filePath)) {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for removeFile command. Source file does not exist.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }

            // TODO: remove file
            // MainLogic::removeSync(filePath); // returns an int that corresponds to something like: 0 = success, 1 = error, 2 = path not being synced, etc...
            std::string responseStr = "";
            int returnValue = 0; // = MainLogic::removeSync(filePath);
            responseStr = "code:" + std::to_string(returnValue) + ":" + std::to_string(commands_to_parse_local[i].second) + "-" + std::to_string(commands_to_parse_local[i].first);
            responses_to_send_local.push_back(std::make_pair(responseStr.size(), responseStr));
            break;
        }
        // - Trigger add directory
        case COMMAND_ADD_DIRECTORY: {
            std::string data = "";
            // first check the corresponding index in data_to_parse_local to see if the slots match
            if (data_to_parse_local[i].first == commands_to_parse_local[i].first) {
                data = data_to_parse_local[i].second;
            } else {
                for (auto& data_pair : data_to_parse_local) {
                    if (data_pair.first == commands_to_parse_local[i].first) {
                        data = data_pair.second;
                        break;
                    }
                }
            }
            if(data == "") {
                GIT_SYNC_D_MESSAGE::Error::error("No data for addDirectory command.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                break;
            }
            // parse data
            // this will be in the format: key:value\nkey:value\nkey:value\n etc...
            // directory: <path to the directory to sync> (c:/users/username/documents/example-repository)
            // destDirectory: <relative path in the destination repository> (subdirectory1/subdirectory2)
            // destRepository: <name of the destination repository> (example-repository)
            // credential: <name of the credential> (example-credential) {optional} - if not specified, the default credential will be used
            // localRepository: <path to the local repository> (c:/users/username/documents/example-repository) {optional} - if not specified and the sync type is sync_with_repo, the directory must be within a repository
            // syncTypeString: <sync type> 4 byte int, result of OR'ing SYNC_TYPE_REPO, SYNC_TYPE_TIME_FRAME, SYNC_TYPE_DIRECTORY, SYNC_TYPE_FILE, etc...
            // syncTimeFrame: <time frame> (1h, 1d, 1w, 1m, 1y, 1h30m, 1d12h, etc...)

            std::string directoryPath = "";
            std::string destDirectoryPath = "";
            std::string destRepository = "";
            std::string credential = "";
            std::string localRepository = "";
            std::string syncTypeString = "";
            std::string syncTimeFrame = "";

            std::vector<std::string> dataLines;
            std::string dataLine;
            for (size_t i = 0; i < data.size(); i++) {
                if (data[i] == '\n') {
                    dataLines.push_back(dataLine);
                    dataLine = "";
                } else {
                    dataLine += data[i];
                }
            }
            if (dataLine != "") {
                dataLines.push_back(dataLine);
            }

            for (auto& line : dataLines) {
                std::string key;
                std::string value;
                bool keyFound = parseKeyValue(line, key, value);
                if (!keyFound) {
                    GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addDirectory command. Unable to parse key/value pair: " + line, GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                    allCommandsParsed = false;
                    continue;
                }
                if (key == "directory")
                    directoryPath = value;
                else if (key == "destDirectory")
                    destDirectoryPath = value;
                else if (key == "destRepository")
                    destRepository = value;
                else if (key == "credential")
                    credential = value;
                else if (key == "localRepository")
                    localRepository = value;
                else if (key == "syncType")
                    syncTypeString = value;
                else if (key == "syncTimeFrame")
                    syncTimeFrame = value;
            }
            if (!allCommandsParsed)
                continue;

            if (directoryPath == "") {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addDirectory command. Unable to parse source directory path.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }

            if (destDirectoryPath == "") {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addDirectory command. Unable to parse destination directory path.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }

            if (destRepository == "") {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addDirectory command. Unable to parse destination repository name.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }

            if (syncTypeString == "") {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addDirectory command. Unable to parse sync type.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }

            // verify that the data is valid
            // - check that the directory exists
            if (!std::filesystem::exists(directoryPath)) {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addDirectory command. Source directory does not exist.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                allCommandsParsed = false;
                continue;
            }

            union {
                char c[4];
                unsigned int i;
            } syncType_un;
            memcpy(syncType_un.c, syncTypeString.c_str(), 4);

            if (syncType_un.i & SYNC_TYPE_REPO) {
                // check that the folder exists within a repo
                if (!withinRepo(directoryPath) && localRepository == "") {
                    GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addDirectory command. Source directory is not within a repository.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                    allCommandsParsed = false;
                    continue;
                }
            }

            size_t syncTime_seconds = 0;
            if (syncType_un.i & SYNC_TYPE_TIME_FRAME) {
                // check that the time frame is valid
                if (!parseTimeFrame(syncTimeFrame, syncTime_seconds)) {
                    GIT_SYNC_D_MESSAGE::Error::error("Error parsing data for addDirectory command. Unable to parse time frame.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                    allCommandsParsed = false;
                    continue;
                }
            }

            // TODO: add directory
            // MainLogic::addDirectory(directoryPath, destDirectoryPath, destRepository, credential, localRepository, syncTime_seconds);
            std::string responseStr = "";
            if (/*the commented call above succeeds*/ true) {
                responseStr = "success:";
            } else {
                responseStr = "error:";
            }
            responseStr += std::to_string(commands_to_parse_local[i].second) + "-" + std::to_string(commands_to_parse_local[i].first);
            responses_to_send_local.push_back(std::make_pair(responseStr.size(), responseStr));
            break;
        }
        // - Trigger remove directory
        // - Trigger add remote repository
        // - Trigger remove remote repository
        // - Trigger add credential
        // - Trigger remove credential
        // - Trigger Sync
        // - Trigger Sync all
        // - Trigger Sync all repositories
        // - Trigger Sync all credentials
        // - Trigger Sync all files
        // - Trigger Sync all directories
        // - Trigger Sync all files and directories
        // - Trigger Sync all files, directories, repositories, and credentials
        // - Trigger Sync all files and directories for a repository
        // - Trigger Sync all files and directories for a credential
        // - Trigger Sync all files and directories for a repository and credential
        // - Read credentials
        // - Read repositories
        // - Read files
        // - Read directories
        // - Read sync types
        case COMMAND_KILL_GIT_SYNC_D:
        {
            std::string responseStr = "";
            responseStr = "success:";
            responseStr += std::to_string(commands_to_parse_local[i].second) + "-" + std::to_string(commands_to_parse_local[i].first);
            responses_to_send_local.push_back(std::make_pair(responseStr.size(), responseStr));
            IPC::shutdown_trigger = true;
            break;
        }
        default: {
            GIT_SYNC_D_MESSAGE::Error::error("Error parsing command. Unknown command code: " + std::to_string(commands_to_parse_local[i].second), GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
            allCommandsParsed = false;
            continue;
            break;
        }
        }
    }
    std::unique_lock<std::mutex> responses_vectors_mutex_lock(responses_vectors_mutex);
    responses_to_send.insert(responses_to_send.end(), responses_to_send_local.begin(), responses_to_send_local.end());
    responses_vectors_mutex_lock.unlock();
    return allCommandsParsed;
}

/**
 * @brief Parse a key/value pair
 *
 * @param input The input string to parse
 * @param key The key
 * @param value The value
 * @return true If the key/value pair was parsed successfully
 * @return false If the key/value pair was not parsed successfully
 */
bool parseKeyValue(std::string& input, std::string& key, std::string& value)
{
    key = "";
    value = "";
    bool keyFound = false;
    for (size_t i = 0; i < input.size(); i++) {
        if (input[i] == ':' && !keyFound) {
            keyFound = true;
            continue;
        }
        if (!keyFound) {
            key += input[i];
        } else {
            value += input[i];
        }
    }
    return keyFound;
}

/**
 * @brief Parse a time frame string
 *
 * @param _input The time frame string to parse (e.g. 1h30m23s)
 * @param time_frame The time frame in seconds
 * @return true If the time frame was parsed successfully
 * @return false If the time frame was not parsed successfully
 */
bool parseTimeFrame(std::string& _input, size_t& time_frame)
{
    // walk throught the string and recursively parse the time frame
    // the time frame is in the format: <number><unit><number><unit><number><unit> etc...
    // <number> is an integer
    // <unit> is one of the following:
    // - s: seconds
    // - m: minutes
    // - h: hours
    // - d: days
    // - w: weeks
    // - M: months
    // - y: years

    // create local copy of input
    std::string input = _input;
    time_frame = 0;
    // first check if the input is empty
    if (input == "") {
        GIT_SYNC_D_MESSAGE::Error::error("Error parsing time frame. Input is empty.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
        return false;
    }

    // check if the input is a number
    bool isNumber = true;
    for (size_t i = 0; i < input.size(); i++) {
        if (!isdigit(input[i])) {
            isNumber = false;
            break;
        }
    }
    if (isNumber) {
        time_frame = std::stoi(input);
        return true;
    }

    std::string numberStr = "";
    std::string unitStr = "";
    bool numberFound = false;
    for (size_t i = 0; i < input.size(); i++) {
        if (isdigit(input[i])) {
            numberFound = true;
            numberStr += input[i];
        } else {
            if (numberFound) {
                unitStr = input[i];
                break;
            } else {
                GIT_SYNC_D_MESSAGE::Error::error("Error parsing time frame. Unable to parse number.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
                return false;
            }
        }
    }
    if (numberStr == "" || unitStr == "") {
        GIT_SYNC_D_MESSAGE::Error::error("Error parsing time frame. Unable to parse number or unit.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
        return false;
    }
    size_t number = std::stoi(numberStr);
    size_t unit = 0;
    switch (unitStr[0]) {
    case 's': {
        unit = 1;
        break;
    }
    case 'm': {
        unit = 60;
        break;
    }
    case 'h': {
        unit = 3600;
        break;
    }
    case 'd': {
        unit = 86400;
        break;
    }
    case 'w': {
        unit = 604800;
        break;
    }
    case 'M': {
        unit = 2592000;
        break;
    }
    case 'y': {
        unit = 31536000;
        break;
    }
    default: {
        GIT_SYNC_D_MESSAGE::Error::error("Error parsing time frame. Unknown unit: " + unitStr, GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
        return false;
    }
    }
    size_t time_frame_local = number * unit;
    std::string recursiveInput = input.substr(numberStr.size() + unitStr.size());
    if (recursiveInput == "") {
        // Base case
        time_frame = time_frame_local;
        return true;
    }
    size_t time_frame_recursive = 0;
    bool recursiveParse = parseTimeFrame(recursiveInput, time_frame_recursive);
    if (!recursiveParse) {
        GIT_SYNC_D_MESSAGE::Error::error("Error parsing time frame. Unable to parse recursive time frame.", GIT_SYNC_D_MESSAGE::IPC_MESSAGE_PARSE_ERROR);
        return false;
    }
    time_frame = time_frame_local + time_frame_recursive;
    return true;
}

/**
 * @brief Check if a path is within a repository
 *
 * @param path The path to check
 * @return true If the path is within a repository
 * @return false If the path is not within a repository
 */
bool withinRepo(std::string& path)
{
    if (path == "") {
        return false;
    }

    std::filesystem::path pathObj(path);

    if (std::filesystem::exists(pathObj / ".git")) {
        return true;
    }

    if (pathObj == pathObj.root_path()) {
        return false;
    }

    // get the parent path and check each folder in that path to see if it is a repo
    std::filesystem::path parentPath;
    try {
        parentPath = std::filesystem::path(path).parent_path();
    } catch (std::exception& e) {
        GIT_SYNC_D_MESSAGE::Error::error("Error getting parent path: " + std::string(e.what()), GIT_SYNC_D_MESSAGE::GENERIC_ERROR);
        return false;
    }
    while (parentPath != parentPath.root_path()) {
        if (std::filesystem::exists(parentPath / ".git")) {
            return true;
        }
        parentPath = parentPath.parent_path();
    }
    return false;
}

/**
 * @brief Restart the pipe
 *
 * @param pipe The pipe to restart
 * @param io_service The io_service to use
 * @param pipe_name The name of the pipe
 */
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
            szSDDL, SDDL_REVISION_1, &pSDDL, NULL)) {
        GIT_SYNC_D_MESSAGE::Error::error("Error converting string to security descriptor: " + std::to_string(GetLastError()), GIT_SYNC_D_MESSAGE::IPC_NAMED_PIPE_ERROR);
        return;
    }
    pSD = (SECURITY_DESCRIPTOR*)pSDDL;
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
        PIPE_BUFFER_SIZE, // outbound buffer
        PIPE_BUFFER_SIZE, // inbound buffer
        0, // use default wait time
        &sa // use default security attributes
    );
    if (pipe_handle == INVALID_HANDLE_VALUE) {
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

#if defined(UNIT_TESTING)
// Do unit tests stuff
#endif