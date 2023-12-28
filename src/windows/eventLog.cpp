#include "eventLog.h"

namespace Windows_EventLog
{
    bool tryRegisterWithEventLog()
    {
        // check to see if GitSyncD is already in the registry
        HKEY hKey;
        if(RegOpenKeyExA(HKEY_LOCAL_MACHINE, KEY_PATH, 0, KEY_READ, &hKey) == ERROR_SUCCESS){
            // GitSyncD is already in the registry
            RegCloseKey(hKey);
        }else{
            // GitSyncD is not in the registry
            // add it
            if(RegCreateKeyExA(HKEY_LOCAL_MACHINE, KEY_PATH, 0, NULL, REG_OPTION_NON_VOLATILE, KEY_WRITE, NULL, &hKey, NULL) == ERROR_SUCCESS){
                RegCloseKey(hKey);
            }else{
                std::string message = "Failed to create registry key: " + std::string(KEY_PATH);
                GIT_SYNC_D_ERROR::Error::error(message, GIT_SYNC_D_ERROR::_ErrorCode::SYSTEM_LOG_ERROR);
                return false;
            }
        }

        bool result = false;
        GIT_SYNC_D_ERROR::Error::error("Registering Git Sync'd with the Event Log", GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
        HANDLE hEventSource;
        LPCTSTR lpszStrings[2];
        TCHAR Buffer[80];
        sprintf_s(Buffer, 80, TEXT("Git Sync'd - Registered with the Event Log"));
        hEventSource = RegisterEventSourceA(NULL, "GitSyncD");
        if (hEventSource!=NULL)
        {
            lpszStrings[0] = Buffer;
            lpszStrings[1] = NULL;
            ReportEvent(hEventSource, // Event log handle
                EVENTLOG_INFORMATION_TYPE, // Event type
                WINDOWS_EVENT_INFORMATION_CATEGORY, // Event category
                WINDOWS_EVENT_MSG_INFORMATION, // Event identifier
                NULL, // No security identifier
                1, // Size of lpszStrings array
                0, // No binary data
                lpszStrings, // Array of strings
                NULL); // No binary data
            result = true;
            DeregisterEventSource(hEventSource);
        }
        else
        {
            std::string message = "RegisterEventSource failed with " + std::to_string(GetLastError());
            GIT_SYNC_D_ERROR::Error::error(message, GIT_SYNC_D_ERROR::_ErrorCode::SYSTEM_LOG_ERROR);
            result = false;
        }
        return result;
    }

    void logEvent(std::string message, GIT_SYNC_D_ERROR::_ErrorCode code)
    {
        HANDLE hEventSource;
        LPCTSTR lpszStrings[1];
        TCHAR Buffer[1024];
        sprintf_s(Buffer, 80, TEXT("%s"), message.c_str());
        hEventSource = RegisterEventSourceA(NULL, "GitSyncD");
        if (hEventSource!=NULL)
        {
            WORD type;
            WORD category;
            DWORD ident;
            switch (code)
            {
            case GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO:
                type = EVENTLOG_INFORMATION_TYPE;
                category = WINDOWS_EVENT_INFORMATION_CATEGORY;
                ident = WINDOWS_EVENT_MSG_INFORMATION;
                break;
            case GIT_SYNC_D_ERROR::_ErrorCode::CODE_NO_ERROR:
                type = EVENTLOG_INFORMATION_TYPE;
                category = WINDOWS_EVENT_INFORMATION_CATEGORY;
                ident = WINDOWS_EVENT_MSG_INFORMATION;
                break;
            default:
                type = EVENTLOG_ERROR_TYPE;
                category = WINDOWS_EVENT_ERROR_CATEGORY;
                ident = WINDOWS_EVENT_MSG_ERROR;
                break;
            }
            lpszStrings[0] = Buffer;
            if(!ReportEvent(hEventSource, // Event log handle
                type, // Event type
                category, // Event category
                ident, // Event identifier
                NULL, // No security identifier
                1, // Size of lpszStrings array
                0, // No binary data
                lpszStrings, // Array of strings
                NULL) // No binary data
            ){
                std::string message = "ReportEvent failed with " + std::to_string(GetLastError());
                GIT_SYNC_D_ERROR::Error::error(message, GIT_SYNC_D_ERROR::_ErrorCode::SYSTEM_LOG_ERROR);
            }
            // unregister the event source
            DeregisterEventSource(hEventSource);
        }
        else
        {
            std::string message = "RegisterEventSource failed with " + std::to_string(GetLastError());
            GIT_SYNC_D_ERROR::Error::error(message, GIT_SYNC_D_ERROR::_ErrorCode::SYSTEM_LOG_ERROR);
        }
    }
}