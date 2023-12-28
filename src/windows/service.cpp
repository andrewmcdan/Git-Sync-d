#ifdef _WIN32
#include "service.h"

namespace Windows_Service
{
    // Define service name
    const TCHAR *serviceName = _T("GitSyncdService");
    const TCHAR *serviceDisplayName = _T("Git Sync'd Service");
    const TCHAR *serviceDescription = _T("Git Sync'd to a remote repository.");

    // Service status structure
    SERVICE_STATUS g_ServiceStatus = {};
    SERVICE_STATUS_HANDLE g_StatusHandle = nullptr;
    HANDLE gh_StopEvent = nullptr;
    std::function<void(std::string, GIT_SYNC_D_ERROR::_ErrorCode)> sysLogEvent;

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

    void InstallService(const TCHAR *exePath)
    {
        SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CREATE_SERVICE);
        if (!scmHandle)
        {
            std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
            return;
        }

        SC_HANDLE serviceHandle = CreateServiceA(
            scmHandle, (LPCSTR)serviceName, (LPCSTR)serviceDisplayName,
            SERVICE_START | SERVICE_STOP | DELETE | SERVICE_QUERY_STATUS | SERVICE_QUERY_CONFIG | SERVICE_PAUSE_CONTINUE,
            SERVICE_WIN32_OWN_PROCESS,
            SERVICE_DEMAND_START, SERVICE_ERROR_NORMAL,
            (LPCSTR)exePath, nullptr, nullptr, nullptr, nullptr, nullptr);

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

    void WINAPI ServiceMain(DWORD argc, LPTSTR *argv)
    {
        LARGE_INTEGER StartingTime, EndingTime, ElapsedMicroseconds;
        LARGE_INTEGER Frequency;
        // Register the handler function for the service
        g_StatusHandle = RegisterServiceCtrlHandler(serviceName, ServiceCtrlHandler);
        if (g_StatusHandle == nullptr)
        {
            // Handle error
            std::cerr << "RegisterServiceCtrlHandler failed: " << GetLastError() << std::endl;
            sysLogEvent("RegisterServiceCtrlHandler failed: " + std::to_string(GetLastError()), GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
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
            // let windows know we've started
            g_ServiceStatus.dwCheckPoint++;
            SetServiceStatus(g_StatusHandle, &g_ServiceStatus);
            // Perform main service function here..
            // get high precision time
            QueryPerformanceCounter(&StartingTime);
            QueryPerformanceCounter(&Frequency);
            MainLogic_H::setLogEvent(sysLogEvent);
            if (!MainLogic_H::loop())
            {
                break;
            } // this is the main service loop.
            QueryPerformanceCounter(&EndingTime);
            // calculate the time it took to run the loop
            ElapsedMicroseconds.QuadPart = EndingTime.QuadPart - StartingTime.QuadPart;
            ElapsedMicroseconds.QuadPart *= 1000000;
            ElapsedMicroseconds.QuadPart /= Frequency.QuadPart;
            // convert to milliseconds
            ElapsedMicroseconds.QuadPart /= 1000;
            // sleep for 100ms minus the time it took to run the loop, but only if it took less than 100ms to run the loop
            // This will ensure that we don't take up too much CPU time unless we need to.
            if (ElapsedMicroseconds.QuadPart < 100)
            {
                Sleep((LONGLONG)100 - ElapsedMicroseconds.QuadPart);
            }
        }
        gh_StopEvent = CreateEvent(nullptr, TRUE, FALSE, nullptr);
        if (gh_StopEvent == NULL)
        {
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
            MainLogic_H::stop();
            while (!MainLogic_H::IsServiceStopped())
            {
                Sleep(100);
            }
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

        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return true;
    }

    bool SetServiceDescription(const TCHAR *serviceName, const TCHAR *serviceDescription)
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
        description.lpDescription = (LPTSTR)serviceDescription;

        if (!ChangeServiceConfig2(serviceHandle, SERVICE_CONFIG_DESCRIPTION, &description))
        {
            std::cerr << "ChangeServiceConfig2 failed: " << GetLastError() << std::endl;
            CloseServiceHandle(serviceHandle);
            CloseServiceHandle(scmHandle);
            return false;
        }

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

        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return true;
    }

    bool StopService()
    {
        SC_HANDLE scmHandle = OpenSCManager(nullptr, nullptr, SC_MANAGER_CONNECT);
        if (!scmHandle)
        {
            std::cerr << "OpenSCManager failed: " << GetLastError() << std::endl;
            return false;
        }

        SC_HANDLE serviceHandle = OpenService(scmHandle, serviceName, SERVICE_STOP);
        if (!serviceHandle)
        {
            std::cerr << "OpenService failed: " << GetLastError() << std::endl;
            CloseServiceHandle(scmHandle);
            return false;
        }

        SERVICE_STATUS status;
        if (!ControlService(serviceHandle, SERVICE_CONTROL_STOP, &status))
        {
            std::cerr << "ControlService failed: " << GetLastError() << std::endl;
            CloseServiceHandle(serviceHandle);
            CloseServiceHandle(scmHandle);
            return false;
        }

        CloseServiceHandle(serviceHandle);
        CloseServiceHandle(scmHandle);
        return true;
    }

    bool IsAdmin()
    {
        BOOL isAdmin = FALSE;
        PSID administratorsGroup;
        SID_IDENTIFIER_AUTHORITY ntAuthority = SECURITY_NT_AUTHORITY;
        if (AllocateAndInitializeSid(&ntAuthority, 2, SECURITY_BUILTIN_DOMAIN_RID,
                                     DOMAIN_ALIAS_RID_ADMINS, 0, 0, 0, 0, 0, 0,
                                     &administratorsGroup))
        {
            CheckTokenMembership(NULL, administratorsGroup, &isAdmin);
            FreeSid(administratorsGroup);
        }
        return isAdmin == TRUE;
    }

    void RestartAsAdmin(int argc, char *argv[])
    {
        char path[MAX_PATH];
        GetModuleFileNameA(NULL, path, MAX_PATH);

        SHELLEXECUTEINFOA shExInfo = {};
        shExInfo.cbSize = sizeof(shExInfo);
        shExInfo.fMask = SEE_MASK_DEFAULT;
        shExInfo.hwnd = 0;
        shExInfo.lpVerb = "runas"; // Run as admin
        shExInfo.lpFile = path;    // Path to current executable
        // fill lpParameters with the command line arguments
        std::string params = "";
        for (int i = 0; i < argc; i++)
        {
            params += argv[i];
            params += " ";
        }
        shExInfo.lpParameters = params.c_str(); // Any parameters
        shExInfo.lpDirectory = 0;
        shExInfo.nShow = SW_NORMAL;

        if (!ShellExecuteExA(&shExInfo))
        {
            DWORD error = GetLastError();
            // Handle error
        }
    }

    void StartWindowsService(int startCode, int argc, char **argv, std::function<void(std::string, GIT_SYNC_D_ERROR::_ErrorCode)> _logEvent)
    {
        sysLogEvent = _logEvent;
        if (startCode == 1 || startCode == 2)
        {
            if (!IsAdmin())
            {
                std::cout << "Not running as admin. Restarting as admin..." << std::endl;
                sysLogEvent("Not running as admin. Restarting as admin...", GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
                RestartAsAdmin(argc, argv);
                return;
            }
            std::cout << "Installing service..." << std::endl;
            sysLogEvent("Installing service...", GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
            if (IsServiceInstalled())
            {
                std::cout << "Service is already installed." << std::endl;
                sysLogEvent("Service is already installed.", GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
                if (startCode == 2)
                {
                    std::cout << "Reinstalling service..." << std::endl;
                    sysLogEvent("Reinstalling service...", GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
                    DeleteService();
                    std::cout << "Service is uninstalled." << std::endl;
                    sysLogEvent("Service is uninstalled.", GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
                }
            }
            else
            {
                std::cout << "Service is not installed." << std::endl;
                sysLogEvent("Service is not installed.", GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
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
        }
        else if (startCode == 3)
        {
            if (IsServiceInstalled())
            {
                std::cout << "Service is installed." << std::endl;
                std::cout << "Attempting to start service..." << std::endl;
                if (StartService())
                {
                    std::cout << "Service started successfully." << std::endl;
                    sysLogEvent("Service already running or started successfully.", GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
                }
                else
                {
                    std::cout << "Service failed to start." << std::endl;
                    sysLogEvent("Service failed to start.", GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
                }
            }
            else
            {
                std::cout << "Service is not installed. Use \"--install\" to install service." << std::endl;
            }
        }
        else if (startCode == 4)
        {
            if (IsServiceInstalled())
            {
                std::cout << "Service is installed." << std::endl;
                std::cout << "Attempting to stop service..." << std::endl;
                if (StopService())
                {
                    std::cout << "Service stopped successfully." << std::endl;
                }
                else
                {
                    std::cout << "Service failed to stop." << std::endl;
                    sysLogEvent("Service failed to stop.", GIT_SYNC_D_ERROR::_ErrorCode::GENERIC_INFO);
                }
            }
            else
            {
                std::cout << "Service is not installed. Use \"--install\" to install service." << std::endl;
            }
        }
        else
        {
            SERVICE_TABLE_ENTRY ServiceTable[] = {
                {(LPSTR)serviceName, (LPSERVICE_MAIN_FUNCTION)ServiceMain},
                {nullptr, nullptr}};
            if (StartServiceCtrlDispatcher(ServiceTable) == FALSE)
            {
                std::cerr << "Error: Could not start the service." << std::endl;
            }
        }
    }
}
#endif
