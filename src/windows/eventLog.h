#pragma once
#ifndef EVENTLOG_H
#define EVENTLOG_H

#define KEY_PATH "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\GitSyncD"
#define _WINSOCKAPI_
#include <string>
#include <vector>
#include <windows.h>
#include <winevt.h>
#include "../common/error.h"
#include "Event Log Resource Files/GitSyncD.h"
namespace Windows_EventLog{
    bool tryRegisterWithEventLog();
    void logEvent(std::string message, GIT_SYNC_D_ERROR::_ErrorCode code);
}
#endif // EVENTLOG_H