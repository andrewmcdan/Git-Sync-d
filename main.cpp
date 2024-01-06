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
    std::cout << "Usage: " << argv[0] << " [--start] [--stop] [--syslog] [--disable-stdout]" << std::endl;
    std::cout << "  --start: Start the service / daemon" << std::endl;
    std::cout << "  --stop: Stop the service / daemon" << std::endl;
    std::cout << "  --syslog: Enable logging to syslog (Windows only)" << std::endl;
    std::cout << "  --disable-stdout: Disable logging to stdout" << std::endl;
    std::cout << "  --help: Display this help message (not implemented)" << std::endl;
    std::cout << "  --version: Display the version (not implemented)" << std::endl;
    std::cout << "  --uninstall: Uninstall the service / daemon (not implemented)" << std::endl;

    int startCode = 2;
    bool syslogEnabled = false;
    bool disableStdout = false;
    if (args.size() > 1) {
        for (auto arg : args) {
            if (arg == "--start") {
                startCode = 2;
            }
            if (arg == "--stop") {
                startCode = 4;
            }
            if (arg == "--syslog") {
                syslogEnabled = true;
            }
            if (arg == "--disable-stdout") {
                disableStdout = true;
            }
        }
    }
    std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> sysLogEvent;
    sysLogEvent = ([&](std::string message, GIT_SYNC_D_MESSAGE::_ErrorCode code){
        message = "Git Sync'd message: " + message + "\n -- Message code:" + std::to_string(code);
        std::cout << message << std::endl;
    });
    GIT_SYNC_D_MESSAGE::Error::setSysLog(sysLogEvent);
#ifdef _WIN32
    if(syslogEnabled && Windows_EventLog::tryRegisterWithEventLog()){
        sysLogEvent = [&](std::string message, GIT_SYNC_D_MESSAGE::_ErrorCode code){
            if(!disableStdout){
                message = "Git Sync'd message: " + message + "\n -- Message code:" + std::to_string(code);
                std::cout << message << std::endl;
            }
            Windows_EventLog::logEvent(message, code);
        };
    }
    Windows_Service::StartWindowsService(startCode, argc, argv, sysLogEvent);
#elif __linux__
    Linux_Daemon::StartLinuxDaemon(startCode, argc, argv, sysLogEvent);
#elif __APPLE__
    Mac_Daemon::StartMacDaemon(startCode, argc, argv, sysLogEvent);
#else
    std::cerr << "Unsupported platform!" << std::endl;
    return 1;
#endif

    return 0;
}
