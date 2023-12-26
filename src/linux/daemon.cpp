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
    execvp("sudo", argv);
}

#endif

