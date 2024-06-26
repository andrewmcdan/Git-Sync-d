#pragma once
#ifndef GIT_SYNC_D_ERROR_H
#define GIT_SYNC_D_ERROR_H
#include <string>
#include <vector>
#include <functional>
#include <mutex>

namespace GIT_SYNC_D_MESSAGE
{
    enum _ErrorCode {
        CODE_NO_ERROR,
        GENERIC_INFO,
        GENERIC_ERROR,
        IPC_MEMORY_MAPPED_FILE_ERROR,
        IPC_MANAGED_SHARED_MEMORY_ERROR,
        IPC_NAMED_PIPE_ERROR,
        SYSTEM_LOG_ERROR,
        IPC_MESSAGE_PARSE_ERROR,
    };
    typedef std::pair<std::string, _ErrorCode> error_t;

    class Error {
    public:
        Error();
        Error(size_t maxErrors);
        ~Error();
        static void error(std::string err1, _ErrorCode code);
        static std::vector<error_t> getErrors();
        static error_t getLastError();
        static std::vector<error_t> errors;
        static size_t maxErrors;
        static size_t getErrorCount();
        static void setSysLog(std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)>);
    private:
        static std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> logEvent;
        static std::mutex mutex_write;
    };
}
#endif // GIT_SYNC_D_ERROR_H