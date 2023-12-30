#pragma once
#ifndef EVENTLOG_H
#define EVENTLOG_H

#define KEY_PATH "SYSTEM\\CurrentControlSet\\Services\\EventLog\\Application\\GitSyncD"

#include <WinSock2.h>
#include <windows.h>
#include <string>
#include <vector>
#include <winevt.h>
#include "../common/error.h"
#include "Event Log Resource Files/GitSyncD.h"
namespace Windows_EventLog {
    bool tryRegisterWithEventLog();
    void logEvent(std::string message, GIT_SYNC_D_ERROR::_ErrorCode code);
}
#endif // EVENTLOG_H