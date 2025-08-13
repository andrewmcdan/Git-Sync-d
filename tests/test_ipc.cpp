#include <catch2/catch_test_macros.hpp>
#include "ipc.h"

TEST_CASE("parseKeyValue splits correctly", "[ipc]") {
    std::string input = "key:value";
    std::string key, value;
    bool ok = parseKeyValue(input, key, value);
    REQUIRE(ok);
    REQUIRE(key == "key");
    REQUIRE(value == "value");
}

TEST_CASE("parseTimeFrame parses mixed units", "[ipc]") {
    std::string input = "1h30m15s";
    size_t seconds = 0;
    bool ok = parseTimeFrame(input, seconds);
    REQUIRE(ok);
    REQUIRE(seconds == 1*3600 + 30*60 + 15);
}
