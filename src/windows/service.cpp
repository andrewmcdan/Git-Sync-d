#ifdef _WIN32
#include "service.h"

namespace Windows_Service
{
    std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> sysLogEvent = nullptr;

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
            if(sysLogEvent!=nullptr){
                GIT_SYNC_D_MESSAGE::Error::error("Failed to restart as admin. Error code: " + std::to_string(error), GIT_SYNC_D_MESSAGE::_ErrorCode::SYSTEM_LOG_ERROR);
            }
        }
    }

    void StartWindowsService(int startCode, int argc, char **argv, std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)> _logEvent)
    {
        sysLogEvent = _logEvent;
        switch(startCode){
            case 1:
                {
                    if (!IsAdmin())
                    {
                        GIT_SYNC_D_MESSAGE::Error::error("Git Sync'd is not running as admin. Restarting as admin.", GIT_SYNC_D_MESSAGE::_ErrorCode::GENERIC_INFO);
                        RestartAsAdmin(argc, argv);
                        return;
                    }
                }
                break;
            case 2:
                MainLogic_H::loop();
                break;
            case 3:
                break;
            case 4:
                break;
            default:
                break;
        }
    }
}
#endif
