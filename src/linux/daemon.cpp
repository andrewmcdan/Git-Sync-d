#ifdef __linux__
 #include "daemon.h"

// TODO: get rid of entire service / daemon code

#include <csignal>
#include <cstdlib>
#include <sys/stat.h>
#include <thread>

namespace Linux_Daemon
{
    std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> sysLogEvent;

    namespace
    {
        void handleSignal(int sig)
        {
            if (sysLogEvent)
            {
                sysLogEvent("Received signal " + std::to_string(sig) + ", shutting down",
                             GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_INFO);
            }
            MainLogic_H::stop();
        }
    }

    void Daemonize()
    {
        pid_t pid = fork();
        if (pid < 0)
            std::exit(EXIT_FAILURE);
        if (pid > 0)
            std::exit(EXIT_SUCCESS); // Parent exits

        umask(0);

        if (setsid() < 0)
            std::exit(EXIT_FAILURE);

        if (chdir("/") < 0)
            std::exit(EXIT_FAILURE);

        close(STDIN_FILENO);
        close(STDOUT_FILENO);
        close(STDERR_FILENO);

        std::signal(SIGTERM, handleSignal);
        std::signal(SIGINT, handleSignal);
    }

    void StartLinuxDaemon(int startCode, int argc, char **argv,
                          std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> logEvent)
    {
        sysLogEvent = logEvent;
        MainLogic_H::setLogEvent(sysLogEvent);

        if (!IsRoot())
        {
            if (sysLogEvent)
            {
                sysLogEvent("Git Sync'd is not running as root. Restarting as root.",
                             GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_INFO);
            }
            RestartAsRoot(argc, argv);
            return;
        }

        Daemonize();

        while (MainLogic_H::loop())
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(200));
        }
    }

    bool IsRoot()
    {
        return geteuid() == 0;
    }

    void RestartAsRoot(int argc, char *argv[])
    {
        execvp("sudo", argv);
    }
}
#endif
