#ifdef __APPLE__
#include "daemon.h"

// TODO: get rid of entire service / daemon code

namespace Mac_Daemon
{
    std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> sysLogEvent;
    void Daemonize()
    {
        // Daemonization process...
    }

    void StartMacDaemon(int startCode, int argc, char** argv, std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> logEvent)
    {
        MainLogic_H::setLogEvent(logEvent);
        sysLogEvent = logEvent;
        Daemonize();
        // Daemon-specific initialization and loop here...
    }

    bool IsRoot()
    {
        return true;
    }

    void RestartAsRoot(int argc, char* argv[])
    {

    }
}
#endif