#include "mainLogic.h"

namespace MainLogic_H {
    bool addFile(const std::string&, const std::string&, const std::string&) { return true; }
    bool addDirectory(const std::string&, const std::string&, const std::string&) { return true; }
    bool removeSync(const std::string&) { return true; }
    void setLogEvent(std::function<void(std::string, GIT_SYNC_D_MESSAGE::_ErrorCode)>) {}
    bool loop() { return true; }
    void stop() {}
    bool IsServiceStopped() { return true; }
}
