#include "error.h"

namespace GIT_SYNC_D_MESSAGE
{
    std::vector<error_t> Error::errors;

    std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> Error::logEvent = nullptr;

    size_t Error::maxErrors = 1024 * 16;

    std::mutex Error::mutex_write;

    Error::Error() {
        Error::logEvent = [](std::string message, GIT_SYNC_D_MESSAGE::_ErrorCode code) {
            // default, do nothing
            };
    }

    Error::Error(size_t maxErrors) {
        Error::maxErrors = maxErrors;
        Error::logEvent = [](std::string message, GIT_SYNC_D_MESSAGE::_ErrorCode code) {
            // default, do nothing
            };
    }

    Error::~Error() {}

    void Error::error(std::string err1, _ErrorCode code) {
        std::lock_guard<std::mutex> locked(Error::mutex_write);
        if (Error::logEvent != nullptr) {
            Error::logEvent(err1, code);
        }
        error_t error;
        error.first = err1;
        error.second = code;
        Error::errors.push_back(error);
        if (Error::errors.size() > Error::maxErrors) {
            Error::errors.erase(Error::errors.begin());
        }
    }

    std::vector<error_t> Error::getErrors() {
        std::lock_guard<std::mutex> lock(Error::mutex_write);
        return Error::errors;
    }

    error_t Error::getLastError() {
        std::lock_guard<std::mutex> lock(Error::mutex_write);
        error_t error;
        error.first = Error::errors.back().first;
        error.second = Error::errors.back().second;
        return error;
    }

    size_t Error::getErrorCount() {
        std::lock_guard<std::mutex> lock(Error::mutex_write);
        if (Error::errors.size() == Error::maxErrors) {
            return -1;
        }
        return Error::errors.size();
    }

    void Error::setSysLog(std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> _logEvent) {
        Error::logEvent = _logEvent;
    }
}