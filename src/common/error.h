#pragma once
#ifndef GIT_SYNC_D_ERROR_H
#define GIT_SYNC_D_ERROR_H
#include <string>
#include <vector>

namespace GIT_SYNC_D_ERROR
{
    enum _ErrorCode {
        GENERIC_INFO,
        IPC_MEMORY_MAPPED_FILE_ERROR,
        IPC_MANAGED_SHARED_MEMORY_ERROR,
        SYSTEM_LOG_ERROR,
    };
    typedef std::pair<std::string, _ErrorCode> error_t;

    class Error{
    public:
        Error();
        Error(int maxErrors);
        ~Error();
        static void error(std::string err1, _ErrorCode code);
        static std::vector<error_t> getErrors();
        static error_t getLastError();
        static std::vector<error_t> errors;
        static int maxErrors;
        static size_t getErrorCount();
    };   
}
#endif // GIT_SYNC_D_ERROR_H