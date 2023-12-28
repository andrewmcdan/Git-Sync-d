#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>
#include <functional>

#ifdef _WIN32
#include "src/windows/service.h"
#include "src/windows/eventLog.h"
#elif __linux__
#include "src/linux/daemon.h"
#elif __APPLE__
#include "src/mac/daemon.h"
#else
#error "Unsupported platform!"
#endif

int main(int argc, char** argv) {
    std::vector<std::string> args;
    if (argc > 0) {
        std::cout << "argc: " << argc << std::endl;
        for (int i = 0; i < argc; i++) {
            std::cout << "argv[" << i << "]: " << argv[i] << std::endl;
            args.push_back(argv[i]);
        }
    }

    std::cout << "Git Sync'd Service / Daemon" << std::endl;
    std::cout << "Usage: " << argv[0] << " [--install] [--reinstall] [--start] [--stop]" << std::endl;

    int startCode = 0;
    if (args.size() > 1) {
        for (auto arg : args) {
            if (arg == "--install") {
                startCode = 1;
            }
            if (arg == "--reinstall") {
                startCode = 2;
            }
            if (arg == "--start") {
                startCode = 3;
            }
            if (arg == "--stop") {
                startCode = 4;
            }
        }
    }
    std::function<void(std::string, GIT_SYNC_D_ERROR::_ErrorCode)> logEvent;
#ifdef _WIN32
    if(Windows_EventLog::tryRegisterWithEventLog()){
        logEvent = Windows_EventLog::logEvent;
    }else{
        logEvent = ([](std::string message, GIT_SYNC_D_ERROR::_ErrorCode code){
            message = "Event log not available. Syslog message: " + message + " -- Error code:" + std::to_string(code);
            GIT_SYNC_D_ERROR::Error::error(message, code);
        });
    }
    Windows_Service::StartWindowsService(startCode, argc, argv, logEvent);
    
#elif __linux__
    StartLinuxDaemon(startCode, argc, argv, logEvent);

#elif __APPLE__
    StartMacDaemon(startCode, argc, argv, logEvent);

#else
    std::cerr << "Unsupported platform!" << std::endl;
    return 1;
#endif

    return 0;
}
