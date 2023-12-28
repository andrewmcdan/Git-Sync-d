#ifdef __linux__
#ifndef LINUX_DAEMON_H
#define LINUX_DAEMON_H

#include <unistd.h>
#include <sys/types.h>
#include <functional>
#include <string>
#include "../common/error.h"
#include "../common/mainLogic.h"

namespace Linux_Daemon{
    void Daemonize();
    bool IsRoot();
    void RestartAsRoot(int argc, char* argv[]);
    void StartLinuxDaemon(int startCode, int argc, char** argv, std::function<void(std::string, GIT_SYNC_D_ERROR::_ErrorCode)> logEvent);
}
#endif // LINUX_DAEMON_H
#endif