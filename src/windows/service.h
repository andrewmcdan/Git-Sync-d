#ifndef WINDOWS_SERVICE_H
#define WINDOWS_SERVICE_H

#include <windows.h>

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv);
VOID WINAPI ServiceCtrlHandler(DWORD fdwControl);
void StartWindowsService(int install);
bool IsServiceInstalled();
void InstallService(const TCHAR* exePath);
bool DeleteService();
bool SetServiceDescription(const char* description);
bool StartService();

#endif // WINDOWS_SERVICE_H