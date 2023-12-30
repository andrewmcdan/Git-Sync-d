#pragma once
#ifndef WINDOWS_SERVICE_H
#define WINDOWS_SERVICE_H
#include <WinSock2.h>
#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include "../common/mainLogic.h"
#include <AccCtrl.h>
#include <Aclapi.h>
#include <functional>

#define SERVICE_CONTROL_USER 128
#define SERVICE_CONTROL_START_CLI (SERVICE_CONTROL_USER + 0)

namespace Windows_Service
{
    void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
    VOID WINAPI ServiceCtrlHandler(DWORD fdwControl);
    void StartWindowsService(int install, int argc, char** argv, std::function<void(std::string, GIT_SYNC_D_ERROR::_ErrorCode)> logEvent);
    bool IsServiceInstalled();
    void InstallService(const TCHAR* exePath);
    bool DeleteService();
    bool SetServiceDescription(const TCHAR* serviceName, const TCHAR* serviceDescription);
    bool StartService();
    bool StopService();
    bool IsAdmin();
    void RestartAsAdmin(int argc, char* argv[]);
}
#endif // WINDOWS_SERVICE_H