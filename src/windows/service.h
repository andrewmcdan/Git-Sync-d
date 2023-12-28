#ifndef WINDOWS_SERVICE_H
#define WINDOWS_SERVICE_H

#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <string>
#include "../common/mainLogic.h"
#include <AccCtrl.h>
#include <Aclapi.h>
#include <functional>
namespace Windows_Service
{
    void WINAPI ServiceMain(DWORD argc, LPTSTR *argv);
    VOID WINAPI ServiceCtrlHandler(DWORD fdwControl);
    void StartWindowsService(int install, int argc, char **argv, std::function<void(std::string, GIT_SYNC_D_ERROR::_ErrorCode)> logEvent);
    bool IsServiceInstalled();
    void InstallService(const TCHAR *exePath);
    bool DeleteService();
    bool SetServiceDescription(const char *description);
    bool StartService();
    bool StopService();
    bool IsAdmin();
    void RestartAsAdmin(int argc, char **argv);
}
#endif // WINDOWS_SERVICE_H