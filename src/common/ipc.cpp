#include "ipc.h"

IPC::IPC(){
    // create a new thread to handle IPC
    // this thread will be responsible for
    // checking for and handling new messages and sending responses

    // create thread
    IPC::running = true;
    this->ipcThread = std::thread([&](){run(*this);});
}

IPC::~IPC(){
    this->running = false;
    shared_memory_object::remove("GitSyncd-sharedMemory");
}

bool IPC::pendingCommands(){
    // check if there are any pending commands
    return this->commands.size() > 0;
}

void run(IPC& _this){
    managed_shared_memory segment = managed_shared_memory(open_or_create, "GitSyncd-sharedMemory", 65536);
    typedef std::pair<std::string, int> command; // command, slot: slot is used as a way to index which command is related to which data.
    typedef std::pair<std::string, int> data; // data, slot: slot is used as a way to index which data is related to which command.
    typedef std::pair<std::string, std::string> response; // command + slot, response: the original command and slot are concatenated to form the key for the response.

    std::pair<command*, managed_shared_memory::size_type> commandVector;
    std::pair<data*, managed_shared_memory::size_type> dataVector;

    std::vector<response> in_memory_responses;
    std::vector<std::string> response_keys;

    while(_this.running){
        commandVector = segment.find<command>("commandVector");

        if(commandVector.first){
            // there are commands to be processed
            for(unsigned int i = 0; i < commandVector.second; i++){
                // process each command
                std::string command = commandVector.first[i].first;
                int slot = commandVector.first[i].second;

                // check if there is data for this command
                dataVector = segment.find<data>("dataVector");
                if(dataVector.first){
                    // there is data for this command
                    for(unsigned int j = 0; j < dataVector.second; j++){
                        // process each data
                        std::string data = dataVector.first[j].first;
                        int dataSlot = dataVector.first[j].second;

                        if(dataSlot == slot){
                            // this data is for this command
                            // process the command and data
                            // and send a response
                            std::string _response = "response";
                            _response += std::to_string(slot);
                            _response += command;
                            _response += data;
                            response *test = segment.construct<response>(_response.c_str())(_response.c_str(),"test");
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
        if(segment.find<response>("response").first){
            for(auto key : response_keys){
                if(key == "response"){
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