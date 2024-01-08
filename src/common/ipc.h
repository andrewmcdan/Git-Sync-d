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
#include <filesystem>

#define PIPE_BUFFER_SIZE 1024 * 16

#define START_PATTERN_STRING "\x11\x22\x33\x44\x33\xA8\xBD\x4E"
#define END_PATTERN_STRING "\x88\x77\x66\x55\xF6\x9C\x29\xD9"

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

enum COMMAND_CODE {
    COMMAND_ADD_FILE,
    COMMAND_REMOVE_FILE,
    COMMAND_ADD_CREDENTIALS,
    COMMAND_REMOVE_CREDENTIALS,
    COMMAND_ADD_REMOTE_REPO,
    COMMAND_REMOVE_REMOTE_REPO,
    COMMAND_ADD_DIRECTORY,
    COMMAND_REMOVE_DIRECTORY,
    COMMAND_ADD_REMOTE_REPO_CREDENTIALS,
    COMMAND_REMOVE_REMOTE_REPO_CREDENTIALS,
    COMMAND_TRIGGER_SYNC,
    COMMAND_TRIGGER_SYNC_ALL,
    COMMAND_TRIGGER_SYNC_ALL_REPOS,
    COMMAND_TRIGGER_SYNC_ALL_CREDENTIALS,
    COMMAND_TRIGGER_SYNC_ALL_DIRECTORIES,
    COMMAND_TRIGGER_SYNC_ALL_FILES,
    COMMAND_TRIGGER_SYNC_ALL_FILES_AND_DIRECTORIES,
    COMMAND_TRIGGER_SYNC_ALL_FILES_AND_DIRECTORIES_FOR_REPO,
    COMMAND_TRIGGER_SYNC_ALL_FILES_AND_DIRECTORIES_FOR_CREDENTIALS,
    COMMAND_TRIGGER_SYNC_ALL_FILES_AND_DIRECTORIES_FOR_REPO_AND_CREDENTIALS,
    COMMAND_TRIGGER_SYNC_ALL_FILES_AND_DIRECTORIES_FOR_REPO_AND_CREDENTIALS_AND_DIRECTORY
};

enum RESPONSE_CODE {
    RESP_SUCCESS,
    RESP_ERROR,
    RESP_UNKNOWN,
    RESP_NOT_IMPLEMENTED,
    RESP_INVALID_COMMAND,
    RESP_INVALID_DATA
};

enum SYNC_TYPE {
    SYNC_TYPE_ALL = 0,
    SYNC_TYPE_REPO = 1,
    SYNC_TYPE_TIME_FRAME = 2,
    SYNC_TYPE_DIRECTORY = 4,
    SYNC_TYPE_FILE = 8,
    SYNC_TYPE_UNDEFINED = 256
};

typedef std::pair<int, COMMAND_CODE> command;  // slot, command: slot is used as a way to index which command is related to which data.
typedef std::pair<int, std::string> data;     // slot, data: slot is used as a way to index which data is related to which command.
typedef std::pair<size_t, std::string> response; // size, response: size of response (not including the size int), the original command and slot are concatenated to form the key for the response.

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
    std::mutex commands_data_vectors_mutex;
    std::mutex responses_vectors_mutex;
    bool running;
    std::mutex running_mutex;
    std::thread ipcThread;
    static bool shutdown_trigger;
    static bool shutdown();
    void startRunThread();
    GIT_SYNC_D_MESSAGE::Error error;
private:

};

bool parseCommands(
    std::vector<command>& commands_to_parse,
    std::vector<data>& data_to_parse,
    std::vector<response>& responses_to_send,
    std::mutex& commands_vectors_mutex,
    std::mutex& responses_vectors_mutex
);
bool parseKeyValue(std::string& input, std::string& keys, std::string& values);
bool parseTimeFrame(std::string& input, size_t& time_frame);
bool withinRepo(std::string& path);
void run(IPC&);

#if defined(BOOST_ASIO_HAS_WINDOWS_STREAM_HANDLE)
void restartPipe(boost::asio::windows::stream_handle& pipe, boost::asio::io_service& io_service, std::string pipe_name);
#elif defined(BOOST_ASIO_HAS_LOCAL_SOCKETS)
void restartPipe(boost::asio::local::stream_protocol::socket& pipe, boost::asio::io_service& io_service, std::string pipe_name);
#endif

#endif // IPC_H