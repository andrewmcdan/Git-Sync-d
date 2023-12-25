#include "mainLogic.h"

namespace MainLogic_H
{
    MainLogic mainLogic;

    void loop()
    {
        // this is the main service loop.
        // it will be called by the service/daemon
        // and will run until the service/daemon is stopped.
        if (mainLogic.isRunning())
        {
            // do all the things
            // 1. check that IPC is active / for new messages

            // 2. check if we have loaded the database

            // 3. check if we have loaded the credentials
        }
        else
        {
            // stop all the things

            // once everything is stopped...
            mainLogic.stopConfirmed();
        }
    }

    void stop()
    {
        mainLogic.stop();
    }

    bool IsServiceStopped()
    {
        return true;
    }

    void MainLogic::setRunning(bool _running)
    {
        mainLogic.running = _running;
    }

    MainLogic::MainLogic()
    {
        // this is the constructor.
        // it will be called by the service/daemon
        // when it is started.
        mainLogic.setRunning(true);
        ipc = new IPC();
    }

    MainLogic::~MainLogic()
    {
        // this is the destructor.
        // it will be called by the service/daemon
        // when it is stopped.
        mainLogic.setRunning(false);
        ipc->~IPC();
    }

    bool MainLogic::isRunning()
    {
        // this is the isRunning method.
        // it will be called by the service/daemon
        // to check if the service/daemon is running.
        return mainLogic.running;
    }

    void MainLogic::stop()
    {
        mainLogic.running = false;
    }

    void MainLogic::stopConfirmed()
    {
        mainLogic.stopped = true;
    }

} // namespace MainLogic_H