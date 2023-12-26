#include "ipc.h"

IPC::IPC()
{
    // create a new thread to handle IPC
    // this thread will be responsible for
    // checking for and handling new messages and sending responses

    // create thread
    this->running = true;
    IPC::shutdown_trigger = false;
    this->ipcThread = std::thread([&]()
        { run(*this); });
}

IPC::~IPC()
{
    this->running = false;
    this->ipcThread.join();
    shared_memory_object::remove("GitSyncd-sharedMemory");
}

bool IPC::pendingCommands()
{
    // check if there are any pending commands
    return this->commands.size() > 0;
}

static bool IPC::shutdown()
{
    return IPC::shutdown_trigger;
}

void run(IPC& _this)
{
#ifdef _WIN32
    // Windows
    // get userdata folder
    std::string userdataFolder;
    try
    {
        PWSTR userDataPath;
        SHGetKnownFolderPath(FOLDERID_ProgramData, 0, NULL, &userDataPath);
        std::wstring wstr(userDataPath);
        userdataFolder = std::string(wstr.begin(), wstr.end());
        userdataFolder += "\\GitSyncd\\";
    }
    catch (...)
    {
        std::cout << "Error getting userdata folder!" << std::endl;
        return;
    }
#elif __linux__
    // Linux
    // get userdata folder
    std::string userdataFolder = getenv("HOME");
    userdataFolder += "/.config/GitSyncd/";
#elif __APPLE__
    // macOS
    // get userdata folder
    std::string userdataFolder = getenv("HOME");
    userdataFolder += "/Library/Application Support/GitSyncd/";
#else
    // Unsupported platform
    std::cerr << "Unsupported platform!" << std::endl;
    return;
#endif
    // check if folder exists
    // if it doesn't, create it
    try
    {
        std::filesystem::create_directories(userdataFolder);
    }
    catch (...)
    {
        std::cout << "Error creating userdata folder!" << std::endl;
        return;
    }
    std::string sharedMemoryFile = userdataFolder + "sharedMemory";
    try
    {

        const std::size_t sharedMemorySize = 65536;
        file_mapping::remove(sharedMemoryFile.c_str());
        std::filebuf fbuf;
        fbuf.open(sharedMemoryFile.c_str(), std::ios_base::in | std::ios_base::out | std::ios_base::trunc | std::ios_base::binary);
        fbuf.pubseekoff(sharedMemorySize - 1, std::ios_base::beg);
        fbuf.sputc(0);
    }
    catch (...)
    {
        std::cout << "Error creating shared memory file!" << std::endl;
        return;
    }

    struct file_remove
    {
        file_remove(const char* filename) : Filename_(filename) {}
        ~file_remove() { file_mapping::remove(Filename_); }
        const char* Filename_;
    } remover(sharedMemoryFile.c_str());
    void* addr;
    try
    {
        file_mapping m_file(sharedMemoryFile.c_str(), read_write);

        mapped_region region(m_file, read_write);

        addr = region.get_address();
        std::size_t size = region.get_size();
    }
    catch (...)
    {
        std::cout << "Error mapping shared memory file!" << std::endl;
        return;
    }

    managed_shared_memory segment = managed_shared_memory(open_or_create, "GitSyncd-sharedMemory", 65536);
    typedef std::pair<std::string, int> command;          // command, slot: slot is used as a way to index which command is related to which data.
    typedef std::pair<std::string, int> data;             // data, slot: slot is used as a way to index which data is related to which command.
    typedef std::pair<std::string, std::string> response; // command + slot, response: the original command and slot are concatenated to form the key for the response.

    command test = std::make_pair("test", 0);

    std::memcpy(addr, &test, sizeof(command));

    std::pair<command*, managed_shared_memory::size_type> commandVector;
    std::pair<data*, managed_shared_memory::size_type> dataVector;

    std::vector<response> in_memory_responses;
    std::vector<std::string> response_keys;

    while (_this.running)
    {

        commandVector = segment.find<command>("commandVector");

        if (commandVector.first)
        {
            // there are commands to be processed
            for (unsigned int i = 0; i < commandVector.second; i++)
            {
                // process each command
                std::string command = commandVector.first[i].first;
                int slot = commandVector.first[i].second;

                // check if there is data for this command
                dataVector = segment.find<data>("dataVector");
                if (dataVector.first)
                {
                    // there is data for this command
                    for (unsigned int j = 0; j < dataVector.second; j++)
                    {
                        // process each data
                        std::string data = dataVector.first[j].first;
                        int dataSlot = dataVector.first[j].second;

                        if (dataSlot == slot)
                        {
                            // this data is for this command
                            // process the command and data
                            // and send a response
                            std::string _response = "response";
                            _response += std::to_string(slot);
                            _response += command;
                            _response += data;
                            response* test = segment.construct<response>(_response.c_str())(_response.c_str(), "test");
                            in_memory_responses.push_back(*test);
                            response_keys.push_back(_response);
                        }
                    }
                }
            }
        }

        // check if there are any responses to be sent
        // if there are, send them

        // check if the responses have been received
        // if they have, remove them
        if (segment.find<response>("response").first)
        {
            for (auto key : response_keys)
            {
                if (key == "response")
                {
                    response_keys.erase(std::remove(response_keys.begin(), response_keys.end(), key), response_keys.end());
                }
            }
        }
        // sleep for a bit
        std::this_thread::sleep_for(std::chrono::milliseconds(100));
    }
}

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