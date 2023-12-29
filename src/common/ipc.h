#pragma once
#ifndef IPC_H
#define IPC_H
#include <string>
#include <vector>
#include <thread>
#include <chrono>
#include <iostream>
#include <fstream>
#include <stdio.h>
#include <stdlib.h>
#include <cstring>


#include "error.h"
#ifdef _WIN32

#endif

class IPC {
public:
    IPC();
    ~IPC();
    bool pendingCommands();
    std::vector<std::string> commands;
    std::vector<std::string> data;
    bool running;
    std::thread ipcThread;
    static bool shutdown_trigger;
    static bool shutdown();
    void startRunThread();
    GIT_SYNC_D_ERROR::Error error;
private:

};

void run(IPC&);

#endif // IPC_H