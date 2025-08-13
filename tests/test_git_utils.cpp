#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>
#include "git.h"

TEST_CASE("findRepoRoot and isRepository", "[git]") {
    auto temp = std::filesystem::temp_directory_path() / "gitutils_test";
    std::filesystem::remove_all(temp);
    std::filesystem::create_directories(temp / "subdir");
    std::filesystem::create_directory(temp / ".git");
    auto nested = temp / "subdir" / "file.txt";
    std::ofstream(nested.string()) << "data";

    REQUIRE(GitUtils::isRepository(temp));
    REQUIRE(GitUtils::findRepoRoot(nested) == temp.string());

    std::filesystem::remove_all(temp);
}

TEST_CASE("non-repository returns empty", "[git]") {
    auto temp = std::filesystem::temp_directory_path() / "gitutils_test2";
    std::filesystem::remove_all(temp);
    std::filesystem::create_directories(temp);

    REQUIRE_FALSE(GitUtils::isRepository(temp));
    REQUIRE(GitUtils::findRepoRoot(temp).empty());

    std::filesystem::remove_all(temp);
}
