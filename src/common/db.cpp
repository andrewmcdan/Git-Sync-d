#include "db.h"
#include "error.h"
#include <sqlite3.h>

namespace DB {

static sqlite3 *db = nullptr;
static std::string currentPath;

static bool execute(const char *sql) {
    char *err = nullptr;
    if (sqlite3_exec(db, sql, nullptr, nullptr, &err) != SQLITE_OK) {
        std::string msg = err ? err : "unknown error";
        sqlite3_free(err);
        GIT_SYNC_D_MESSAGE::Error::error("SQLite exec failed: " + msg,
                                         GIT_SYNC_D_MESSAGE::GENERIC_ERROR);
        return false;
    }
    return true;
}

bool initDB(const std::string &dbPath) {
    if (db)
        return true;
    currentPath = dbPath.empty() ? "git-sync.db" : dbPath;
    if (sqlite3_open(currentPath.c_str(), &db) != SQLITE_OK) {
        GIT_SYNC_D_MESSAGE::Error::error(
            "Failed to open database: " + std::string(sqlite3_errmsg(db)),
            GIT_SYNC_D_MESSAGE::GENERIC_ERROR);
        return false;
    }
    const char *createSql =
        "CREATE TABLE IF NOT EXISTS sync_entries ("
        "id INTEGER PRIMARY KEY AUTOINCREMENT,"
        "file_path TEXT NOT NULL,"
        "repo_path TEXT NOT NULL,"
        "options TEXT"
        ");";
    return execute(createSql);
}

bool addSyncEntry(const std::string &filePath,
                  const std::string &repoPath,
                  const std::string &options) {
    if (!db && !initDB())
        return false;
    const char *sql =
        "INSERT INTO sync_entries (file_path, repo_path, options) VALUES (?,?,?);";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        GIT_SYNC_D_MESSAGE::Error::error(
            "SQLite prepare failed: " + std::string(sqlite3_errmsg(db)),
            GIT_SYNC_D_MESSAGE::GENERIC_ERROR);
        return false;
    }
    sqlite3_bind_text(stmt, 1, filePath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 2, repoPath.c_str(), -1, SQLITE_TRANSIENT);
    sqlite3_bind_text(stmt, 3, options.c_str(), -1, SQLITE_TRANSIENT);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (!ok) {
        GIT_SYNC_D_MESSAGE::Error::error(
            "SQLite insert failed: " + std::string(sqlite3_errmsg(db)),
            GIT_SYNC_D_MESSAGE::GENERIC_ERROR);
    }
    sqlite3_finalize(stmt);
    return ok;
}

bool removeSyncEntry(int id) {
    if (!db && !initDB())
        return false;
    const char *sql = "DELETE FROM sync_entries WHERE id=?;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        GIT_SYNC_D_MESSAGE::Error::error(
            "SQLite prepare failed: " + std::string(sqlite3_errmsg(db)),
            GIT_SYNC_D_MESSAGE::GENERIC_ERROR);
        return false;
    }
    sqlite3_bind_int(stmt, 1, id);
    bool ok = sqlite3_step(stmt) == SQLITE_DONE;
    if (!ok) {
        GIT_SYNC_D_MESSAGE::Error::error(
            "SQLite delete failed: " + std::string(sqlite3_errmsg(db)),
            GIT_SYNC_D_MESSAGE::GENERIC_ERROR);
    }
    sqlite3_finalize(stmt);
    return ok;
}

std::vector<SyncEntry> listSyncEntries() {
    std::vector<SyncEntry> entries;
    if (!db && !initDB())
        return entries;
    const char *sql =
        "SELECT id, file_path, repo_path, options FROM sync_entries;";
    sqlite3_stmt *stmt = nullptr;
    if (sqlite3_prepare_v2(db, sql, -1, &stmt, nullptr) != SQLITE_OK) {
        GIT_SYNC_D_MESSAGE::Error::error(
            "SQLite prepare failed: " + std::string(sqlite3_errmsg(db)),
            GIT_SYNC_D_MESSAGE::GENERIC_ERROR);
        return entries;
    }
    while (sqlite3_step(stmt) == SQLITE_ROW) {
        SyncEntry e;
        e.id = sqlite3_column_int(stmt, 0);
        const unsigned char *fp = sqlite3_column_text(stmt, 1);
        const unsigned char *rp = sqlite3_column_text(stmt, 2);
        const unsigned char *op = sqlite3_column_text(stmt, 3);
        e.filePath = fp ? reinterpret_cast<const char *>(fp) : "";
        e.repoPath = rp ? reinterpret_cast<const char *>(rp) : "";
        e.options = op ? reinterpret_cast<const char *>(op) : "";
        entries.push_back(e);
    }
    sqlite3_finalize(stmt);
    return entries;
}

} // namespace DB
