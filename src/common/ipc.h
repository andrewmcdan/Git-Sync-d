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
#include <cstdlib>
#include <mutex>

#define PIPE_BUFFER_SIZE 1024

#define USE_BOOST_ASIO
#include <boost/asio.hpp>
#include <boost/system/error_code.hpp>
#include <boost/bind/bind.hpp>

#if defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
#include <boost/asio/local/stream_protocol.hpp>
#elif defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
#include <boost/asio/windows/stream_handle.hpp>
#endif

#include "error.h"
#ifdef _WIN32
#include <Aclapi.h>
#include <Sddl.h>
#endif

class IPC {
public:
    IPC();
    ~IPC();
    bool pendingCommands();
    std::vector<std::string> commands;
    std::vector<std::string> data;
    bool running;
    std::mutex running_mutex;
    std::thread ipcThread;
    static bool shutdown_trigger;
    static bool shutdown();
    void startRunThread();
    GIT_SYNC_D_ERROR::Error error;
private:

};

void run(IPC&);

#endif // IPC_H