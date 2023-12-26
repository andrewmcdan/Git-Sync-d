#ifndef LINUX_DAEMON_H
#define LINUX_DAEMON_H

#include <unistd.h>
#include <sys/types.h>

void StartLinuxDaemon(int install, int argc, char** argv);

#endif // LINUX_DAEMON_H
