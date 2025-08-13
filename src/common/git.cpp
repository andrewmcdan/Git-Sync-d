#include "git.h"

#include <array>
#include <cstdio>

#ifdef _WIN32
#define popen _popen
#define pclose _pclose
#endif

namespace GitUtils {

std::string findRepoRoot(const std::filesystem::path &start) {
    std::filesystem::path current = start;
    if (std::filesystem::is_regular_file(current)) {
        current = current.parent_path();
    }
    while (!current.empty()) {
        if (std::filesystem::exists(current / ".git")) {
            return current.string();
        }
        if (current == current.root_path()) {
            break;
        }
        current = current.parent_path();
    }
    return "";
}

bool isRepository(const std::filesystem::path &path) {
    return !findRepoRoot(path).empty();
}

std::string getLastCommitMessage(const std::string &repoRoot,
                                const std::string &filePath,
                                int maxLen) {
    if (repoRoot.empty()) {
        return "";
    }

    std::string command = "git -C \"" + repoRoot + "\" log -1 --pretty=%B";
    if (!filePath.empty()) {
        command += " -- \"" + filePath + "\"";
    }

    std::array<char, 256> buffer{};
    std::string result;
    FILE *pipe = popen(command.c_str(), "r");
    if (!pipe) {
        return "";
    }
    while (fgets(buffer.data(), static_cast<int>(buffer.size()), pipe) != nullptr) {
        result += buffer.data();
    }
    pclose(pipe);

    while (!result.empty() && (result.back() == '\n' || result.back() == '\r')) {
        result.pop_back();
    }
    if (maxLen > 0 && static_cast<int>(result.size()) > maxLen) {
        result = result.substr(0, maxLen);
    }
    return result;
}

} // namespace GitUtils

