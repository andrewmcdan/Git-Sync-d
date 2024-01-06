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
    
}
#endif // MAINLOGIC_H