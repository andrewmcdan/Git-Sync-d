#ifdef __APPLE__
#ifndef MAC_DAEMON_H
#define MAC_DAEMON_H

#include <functional>
#include <string>
#include "../common/error.h"
#include "../common/mainLogic.h"

namespace Mac_Daemon{
    void Daemonize();
    bool IsRoot();
    void RestartAsRoot(int argc, char* argv[]);
    void StartMacDaemon(int startCode, int argc, char** argv, std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> logEvent);
}

#endif // MAC_DAEMON_H
#endif