#ifdef _WIN32
#include "service.h"
#include <windows.h>
#include <tchar.h>
#include <iostream>
#include <string>

// Define service name
const TCHAR* serviceName = _T("GitSyncdService");
const TCHAR* serviceDisplayName = _T("Git Sync'd Service");
const TCHAR* serviceDescription = _T("Git Sync'd to a remote repository.");

// Service status structure
SERVICE_STATUS g_ServiceStatus = {};
SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
HANDLE gh_StopEvent = nullptr;

bool IsServiceInstalled()
{
    SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scmHandle)
    {
        return false;
    }

    SC_HANDLE serviceHandle = OpenService(scmHandle, serviceName, SERVICE_QUERY_STATUS);
    if (serviceHandle)
    {
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return true;
    }

    CloseServiceHandle(scmHandle);
    return false;
}

void InstallService(const TCHAR* exePath)
{
    SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
    if (!scmHandle)
    {
        std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
        return;
    }

    SC_HANDLE serviceHandle = CreateServiceA(
        scmHandle, (LPCSTR) serviceName, (LPCSTR) serviceDisplayName,
        SERVICE_START | SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_PAUSE_CONTINUE,
        SERVICE_WIN32_OWN_PROCESS,
        SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
        (LPCSTR) exePath, nullptr, nullptr, nullptr, nullptr, nullptr);

    if (!serviceHandle)
    {
        std::cerr << "CreateService failed: " << GetLastError() << std::endl;
        CloseServiceHandle(scmHandle);
        return;
    }

    std::cout << "Service installed successfully." << std::endl;

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
}

void WINAPI ServiceMain(DWORD argc, LPTSTR* argv)
{
    // Register the handler function for the service
    g_StatusHandle = RegisterServiceCtrlHandler(serviceName, ServiceCtrlHandler);
    if (g_StatusHandle == nullptr)
    {
        // Handle error

        return;
    }

    // Set service status to running
    g_ServiceStatus.dwServiceType = SERVICE_WIN32_OWN_PROCESS;
    g_ServiceStatus.dwCurrentState = SERVICE_RUNNING;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

    // Service run loop
    while (g_ServiceStatus.dwCurrentState == SERVICE_RUNNING)
    {
        // set dwControlsAccepted to accept SERVICE_CONTROL_STOP requests
        g_ServiceStatus.dwControlsAccepted = SERVICE_ACCEPT_STOP;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

        // TODO: Perform main service function here...

        // let windows know we've started
        g_ServiceStatus.dwCheckPoint++;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);

        // For demo purposes, we'll just sleep
        Sleep(3000); // Sleep for 3 seconds
    }
    gh_StopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
    if (gh_StopEvent == NULL)
    {..
        return;
    }
    // Signal the service to stop
    g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
    SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
}

VOID WINAPI ServiceCtrlHandler(DWORD fdwControl)
{
    // Control request handling logic...
    switch (fdwControl)
    {
    case SERVICE_CONTROL_SHUTDOWN:
    case SERVICE_CONTROL_STOP:
        // Signal that the service is stopping
        g_ServiceStatus.dwCurrentState = SERVICE_STOP_PENDING;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        // TODO: Perform tasks necessary to stop the service here...

        // Signal the service to stop
        g_ServiceStatus.dwCurrentState = SERVICE_STOPPED;
        SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
        break;
    case SERVICE_CONTROL_INTERROGATE:
        break;
    default:
        break;
    }
}

bool DeleteService()
{
    SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scmHandle)
    {
        std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
        return false;
    }

    SC_HANDLE serviceHandle = OpenService(scmHandle, serviceName, SERVICE_STOP | DELETE);
    if (!serviceHandle)
    {
        std::cerr << "OpenService failed: " << GetLastError() << std::endl;
        CloseServiceHandle(scmHandle);
        return false;
    }

    if (!DeleteService(serviceHandle))
    {
        std::cerr << "DeleteService failed: " << GetLastError() << std::endl;
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return false;
    }

    std::cout << "Service deleted successfully." << std::endl;

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
    return true;
}

bool SetServiceDescription(const TCHAR* serviceName, const TCHAR* serviceDescription)
{
    SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scmHandle)
    {
        std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
        return false;
    }

    SC_HANDLE serviceHandle = OpenService(scmHandle, serviceName, SERVICE_CHANGE_CONFIG);
    if (!serviceHandle)
    {
        std::cerr << "OpenService failed: " << GetLastError() << std::endl;
        CloseServiceHandle(scmHandle);
        return false;
    }

    SERVICE_DESCRIPTION description;
    description.lpDescription = (LPTSTR) serviceDescription;

    if (!ChangeServiceConfig2(serviceHandle, SERVICE_CONFIG_DESCRIPTION, &description))
    {
        std::cerr << "ChangeServiceConfig2 failed: " << GetLastError() << std::endl;
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return false;
    }

    std::cout << "Service description set successfully." << std::endl;

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
    return true;
}

bool StartService()
{
    SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
    if (!scmHandle)
    {
        std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
        return false;
    }

    SC_HANDLE serviceHandle = OpenService(scmHandle, serviceName, SERVICE_START);
    if (!serviceHandle)
    {
        std::cerr << "OpenService failed: " << GetLastError() << std::endl;
        CloseServiceHandle(scmHandle);
        return false;
    }

    if (!StartService(serviceHandle, 0, nullptr))
    {
        std::cerr << "StartService failed: " << GetLastError() << std::endl;
        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return false;
    }

    std::cout << "Service started successfully." << std::endl;

    CloseServiceHandle(serviceHandle);
    CloseServiceHandle(scmHandle);
    return true;
}

void StartWindowsService(int install)
{
    if (install > 0)
    {
        std::cout << "Installing service..." << std::endl;
        if (IsServiceInstalled())
        {
            std::cout << "Service is already installed." << std::endl;
            if (install == 2)
            {
                std::cout << "Reinstalling service..." << std::endl;
                DeleteService();
                std::cout << "Service is uninstalled." << std::endl;
            }
        } else
        {
            std::cout << "Service is not installed." << std::endl;
        }
        // get the current working directory
        TCHAR path[MAX_PATH];
        GetCurrentDirectory(MAX_PATH, path);
        std::cout << "Current working directory: " << path << std::endl;
        // get the path to the executable
        TCHAR exePath[MAX_PATH];
        GetModuleFileName(nullptr, exePath, MAX_PATH);
        std::cout << "Executable path: " << exePath << std::endl;
        // install the service
        InstallService(exePath);
        // set the service description
        SetServiceDescription(serviceName, serviceDescription);
        // start the service
        StartService();
    } else
    {
        SERVICE_TABLE_ENTRY ServiceTable[] = {
            {(LPSTR) serviceName, (LPSERVICE_MAIN_FUNCTION) ServiceMain},
            {nullptr, nullptr} };
        if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
        {
            std::cerr << "Error: Could not start the service." << std::endl;
        }
    }
}

#endif
