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

#define PIPE_BUFFER_SIZE 1024 * 16

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


typedef std::pair<int, std::string> command;  // command, slot: slot is used as a way to index which command is related to which data.
typedef std::pair<int, std::string> data;     // data, slot: slot is used as a way to index which data is related to which command.
typedef std::pair<int, std::string> response; // size, response: size of response, the original command and slot are concatenated to form the key for the response.

class IPC {
public:
    IPC();
    ~IPC();
    bool pendingCommands();
    std::vector<command> commands_to_parse;
    std::vector<command> commands_parsed;
    std::vector<data> data_to_parse;
    std::vector<data> data_parsed;
    std::vector<response> responses_to_send;
    std::mutex commands_vectors_mutex;
    bool running;
    std::mutex running_mutex;
    std::thread ipcThread;
    static bool shutdown_trigger;
    static bool shutdown();
    void startRunThread();
    GIT_SYNC_D_MESSAGE::Error error;
private:

};

void run(IPC&);
#if defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
void restartPipe(boost::asio::windows::stream_handle& pipe, boost::asio::io_service& io_service, std::string pipe_name);
#elif defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
void restartPipe(boost::asio::local::stream_protocol::socket& pipe, boost::asio::io_service& io_service, std::string pipe_name);
#endif


#endif // IPC_H