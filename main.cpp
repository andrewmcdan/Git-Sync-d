#include <iostream>
#include <string>
#include <cstdlib>
#include <vector>


#ifdef _WIN32
#include "src/windows/service.h"
#elif __linux__
#include "src/linux/daemon.h"
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
#ifdef _WIN32
    StartWindowsService(startCode);

#elif __linux__
    StartLinuxDaemon(startCode);

#else
    std::cerr << "Unsupported platform!" << std::endl;
    return 1;
#endif

    return 0;
}
