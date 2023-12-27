#pragma once
#ifndef IPC_H
#define IPC_H
#include <boost/interprocess/managed_shared_memory.hpp>
#include <boost/interprocess/file_mapping.hpp>
#include <iostream>
#include <cstddef>
#include <cstdlib>
#include <cstring>
#include <string>
#include <vector>
#include <cassert>
#include <utility>
#include <thread>
#include <fstream>
#include <filesystem>
#include "error.h"
#ifdef _WIN32
#include <windows.h>
#include <shlobj_core.h>
#endif

using namespace boost::interprocess;
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
    struct shm_remove {
        shm_remove() { shared_memory_object::remove("GitSyncd-sharedMemory"); }
        ~shm_remove() { shared_memory_object::remove("GitSyncd-sharedMemory"); }
    } remover;
};

void run(IPC&);

#endif // IPC_H