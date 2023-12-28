#ifdef __linux__
#include "daemon.h"

void Daemonize() {
    // Daemonization process...
}

void StartLinuxDaemon(int startCode, int argc, char** argv, std::function<void(std::string, GIT_SYNC_D_ERROR::_ErrorCode)> logEvent) {
    Daemonize();
    // Daemon-specific initialization and loop here...
}

bool IsRoot() {
    return geteuid() == 0;
}


void RestartAsRoot(int argc, char* argv[]) {
    execvp("sudo", argv);
}

#endif

