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
    if(argc > 0){
        std::cout << "argc: " << argc << std::endl;
        for(int i = 0; i < argc; i++){
            std::cout << "argv[" << i << "]: " << argv[i] << std::endl;
            args.push_back(argv[i]);
        }
    }
    std::string message = "Hello, world!";
    std::cout << message << std::endl;

    int install = 0;
    if(args.size() > 1){
        for(auto arg : args){
            if(arg == "--install"){
                install = 1;
            }
            if(arg == "--reinstall"){
                install = 2;
            }
        }
    }
    #ifdef _WIN32
    StartWindowsService(install);

    #elif __linux__
    StartLinuxDaemon(install);

    #else
    std::cerr << "Unsupported platform!" << std::endl;
    return 1;
    #endif

    return 0;
}
