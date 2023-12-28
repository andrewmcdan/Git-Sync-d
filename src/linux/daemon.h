#ifndef LINUX_DAEMON_H
#define LINUX_DAEMON_H

#include <unistd.h>
#include <sys/types.h>
#include <functional>
#include <string>
#include "../common/error.h"

void StartLinuxDaemon(int startCode, int argc, char** argv, std::function<void(std::string, GIT_SYNC_D_ERROR::_ErrorCode)> logEvent);

#endif // LINUX_DAEMON_H
