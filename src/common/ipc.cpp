#include "ipc.h"

bool IPC::shutdown_trigger = false;

IPC::IPC()
{
    IPC::shutdown_trigger = false;
    this->startRunThread();
}

IPC::~IPC()
{
    this->running = false;
    this->ipcThread.join();
}

void IPC::startRunThread()
{
    this->running = true;
    this->ipcThread = std::thread([&]()
        { run(*this); });
}

bool IPC::pendingCommands()
{
    // check if there are any pending commands
    return this->commands.size() > 0;
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

    // we're gonna be launching the CLI and the GUI from the service, so we can use the managed_shared_memory class.


    typedef std::pair<int, std::string> command;          // command, slot: slot is used as a way to index which command is related to which data.
    typedef std::pair<int, std::string> data;             // data, slot: slot is used as a way to index which data is related to which command.
    typedef std::pair<int, std::string> response; // size, response: size of response, the original command and slot are concatenated to form the key for the response.
    while (_this.running)
    {
        // check async_read for new commands


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