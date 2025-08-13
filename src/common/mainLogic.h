#pragma once
#ifndef MAINLOGIC_H
#define MAINLOGIC_H
#include "git.h"
#include "db.h"
#include "credentials.h"
#include "ipc.h"
#include "error.h"
#include <string>
#include <functional>
#include <iostream>
#include <thread>
namespace MainLogic_H
{
    class MainLogic
    {
    public:
        MainLogic();
        ~MainLogic();
        bool isRunning();
        void setRunning(bool);
        void stop();
        void stopConfirmed();
        bool stopped;
        IPC* ipc;
    private:
        bool running;
    };

    bool loop();
    void stop();
    bool IsServiceStopped();
    void setLogEvent(std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)>);

    // Add or remove file/directory synchronisation entries in the database.
    // These functions also restart any file watchers so that changes take
    // effect immediately.
    bool addFile(const std::string &filePath,
                 const std::string &repoPath,
                 const std::string &options);

    bool addDirectory(const std::string &dirPath,
                      const std::string &repoPath,
                      const std::string &options);

    // Remove a sync entry (file or directory) identified by its path.
    bool removeSync(const std::string &path);
    
}
#endif // MAINLOGIC_H
