#ifndef MAC_DAEMON_H
#define MAC_DAEMON_H

#include <functional>
#include <string>
#include "../common/error.h"

void StartMacDaemon(int startCode, int argc, char** argv, std::function<void(std::string, GIT_SYNC_D_ERROR::_ErrorCode)> logEvent);

#endif // MAC_DAEMON_H
