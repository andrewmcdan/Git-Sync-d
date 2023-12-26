#ifdef __linux__
#include "daemon.h"
#include <unistd.h>
#include <sys/stat.h>
#include <syslog.h>

void Daemonize() {
    // Daemonization process...
}

void StartLinuxDaemon() {
    Daemonize();
    // Daemon-specific initialization and loop here...
}



bool IsRoot() {
    return geteuid() == 0;
}


void RestartAsRoot(int argc, char* argv[]) {
    char* args[argc + 2];
    args[0] = "sudo";
    args[1] = argv[0]; // Path to current executable

    for (int i = 1; i < argc; ++i) {
        args[i + 1] = argv[i];
    }
    args[argc + 1] = NULL;

    execvp("sudo", args);

    // Handle error if execvp returns
}


#endif
