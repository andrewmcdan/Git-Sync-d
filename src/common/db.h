#pragma once
#ifndef GIT_SYNC_D_DB_H
#define GIT_SYNC_D_DB_H

#include <string>
#include <vector>

namespace DB {

struct SyncEntry {
    int id;
    std::string filePath;
    std::string repoPath;
    std::string options;
};

// Initialise the database. The database file will be created if it does not
// already exist. When `dbPath` is empty a default file named `git-sync.db` in
// the current working directory is used.
bool initDB(const std::string &dbPath = "");

// Insert a new synchronisation entry.
bool addSyncEntry(const std::string &filePath,
                  const std::string &repoPath,
                  const std::string &options);

// Remove an entry identified by `id`.
bool removeSyncEntry(int id);

// List all synchronisation entries.
std::vector<SyncEntry> listSyncEntries();

} // namespace DB

#endif // GIT_SYNC_D_DB_H
