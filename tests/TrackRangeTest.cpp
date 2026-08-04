#include "optigrab/domain/Errors.hpp"
#include "optigrab/domain/TrackRange.hpp"

#include <catch2/catch_test_macros.hpp>

using optigrab::parseTrackRange;
using optigrab::ParseError;

TEST_CASE("parseTrackRange all", "[trackrange]") {
    const auto t = parseTrackRange("all", 3);
    REQUIRE(t == std::vector<int>{1, 2, 3});
}

TEST_CASE("parseTrackRange single", "[trackrange]") {
    REQUIRE(parseTrackRange("2", 5) == std::vector<int>{2});
}

TEST_CASE("parseTrackRange range and list", "[trackrange]") {
    REQUIRE(parseTrackRange("1-3,5", 6) == std::vector<int>{1, 2, 3, 5});
}

TEST_CASE("parseTrackRange rejects out of range", "[trackrange]") {
    REQUIRE_THROWS_AS(parseTrackRange("9", 3), ParseError);
    REQUIRE_THROWS_AS(parseTrackRange("3-1", 5), ParseError);
}
