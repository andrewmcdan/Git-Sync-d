#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include "db.h"

TEST_CASE("DB add and remove entries", "[db]") {
    auto dbPath = std::filesystem::temp_directory_path() / "git_sync_test.db";
    std::filesystem::remove(dbPath);
    REQUIRE(DB::initDB(dbPath.string()));
    REQUIRE(DB::addSyncEntry("file.txt", "repo", "opts"));

    auto entries = DB::listSyncEntries();
    REQUIRE(entries.size() == 1);
    REQUIRE(entries[0].filePath == "file.txt");
    int id = entries[0].id;
    REQUIRE(DB::removeSyncEntry(id));
    REQUIRE(DB::listSyncEntries().empty());
}
