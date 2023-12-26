#pragma once
#ifndef MAINLOGIC_H
#define MAINLOGIC_H
#include "git.h"
#include "db.h"
#include "credentials.h"
#include "ipc.h"
#include <iostream>
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
}
#endif // MAINLOGIC_H