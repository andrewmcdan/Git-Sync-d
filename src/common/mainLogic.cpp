#include "mainLogic.h"

namespace MainLogic_H
{
    GIT_SYNC_D_ERROR::Error globalErrors(1000);
    MainLogic mainLogic;

    bool loop()
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


            // check to see if we have we received a shutdown command
            if(!mainLogic.ipc->running){
                if(!GIT_SYNC_D_ERROR::Error::getLastError().second == GIT_SYNC_D_ERROR::IPC_MEMORY_MAPPED_FILE_ERROR)
                    mainLogic.ipc->startRunThread();
                else{
                    // TODO: handle this error by logging to the system log
                }
            }
            if (IPC::shutdown())
            {
                mainLogic.stop();
            }
        } else
        {
            // stop all the things

            // once everything is stopped...
            mainLogic.stopConfirmed();
            return false; // this will break the service / daemon out of its loop
        }
        return true;
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
        mainLogic.setRunning(true);
        ipc = new IPC();
    }

    MainLogic::~MainLogic()
    {
        mainLogic.setRunning(false);
        ipc->~IPC();
    }

    bool MainLogic::isRunning()
    {
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