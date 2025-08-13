#pragma once
#ifndef GIT_SYNC_D_GIT_H
#define GIT_SYNC_D_GIT_H

#include <filesystem>
#include <string>

namespace GitUtils {

// Return the path to the repository root that contains `path`.
// If `path` is not inside a repository, an empty string is returned.
std::string findRepoRoot(const std::filesystem::path &path);

// Check whether `path` resides within a Git repository.
bool isRepository(const std::filesystem::path &path);

// Retrieve the latest commit message for `filePath` inside `repoRoot`.
// If `filePath` is empty, the latest commit message for the repository is returned.
// The result is truncated to `maxLen` characters when `maxLen` is positive.
std::string getLastCommitMessage(const std::string &repoRoot,
                                const std::string &filePath = "",
                                int maxLen = 0);

// Stage `filePath` in the repository located at `repoRoot`.
// Returns the Git CLI exit code.
int stageFile(const std::string &repoRoot, const std::string &filePath);

// Commit staged changes in `repoRoot` using `message`.
// Returns the Git CLI exit code.
int commit(const std::string &repoRoot, const std::string &message);

// Push committed changes from `repoRoot` to `remoteName`.
// Returns the Git CLI exit code.
int push(const std::string &repoRoot, const std::string &remoteName);

} // namespace GitUtils

#endif // GIT_SYNC_D_GIT_H

