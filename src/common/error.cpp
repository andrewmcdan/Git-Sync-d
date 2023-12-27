#include "error.h"

namespace GIT_SYNC_D_ERROR
{
    std::vector<error_t> Error::errors;
    int Error::maxErrors = 1000;
    Error::Error(){}
    Error::Error(int maxErrors){
        Error::maxErrors = maxErrors;
    }
    Error::~Error(){}
    void Error::error(std::string err1, _ErrorCode code){
        error_t error;
        error.first = err1;
        error.second = code;
        Error::errors.push_back(error);
        if(Error::errors.size() > Error::maxErrors){
            Error::errors.erase(Error::errors.begin());
        }
    }
    std::vector<error_t> Error::getErrors(){
        return Error::errors;
    }
    error_t Error::getLastError(){
        error_t error;
        error.first = Error::errors.back().first;
        error.second = Error::errors.back().second;
        return error;
    }
    size_t Error::getErrorCount(){
        if(Error::errors.size() == Error::maxErrors){
            return -1;
        }
        return Error::errors.size();
    }
}