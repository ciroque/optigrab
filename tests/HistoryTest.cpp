#include "optigrab/cli/History.hpp"

#include <catch2/catch_test_macros.hpp>

using optigrab::History;

TEST_CASE("History ignores empty and consecutive duplicates", "[history]") {
    History h;
    h.add("");
    REQUIRE(h.empty());

    h.add("list drive");
    h.add("list drive");
    REQUIRE(h.size() == 1);

    h.add("select drive 0");
    REQUIRE(h.size() == 2);
}

TEST_CASE("History older walks backward and clamps at oldest", "[history]") {
    History h;
    h.add("one");
    h.add("two");
    h.add("three");

    REQUIRE(h.older("draft") == "three");
    REQUIRE(h.older("draft") == "two");
    REQUIRE(h.older("draft") == "one");
    REQUIRE(h.older("draft") == "one");  // clamp
    REQUIRE(h.isNavigating());
}

TEST_CASE("History newer returns draft after leaving history", "[history]") {
    History h;
    h.add("one");
    h.add("two");

    REQUIRE(h.older("partial") == "two");
    REQUIRE(h.older("partial") == "one");
    REQUIRE(h.newer() == "two");
    REQUIRE(h.newer() == "partial");
    REQUIRE_FALSE(h.isNavigating());
    REQUIRE_FALSE(h.newer().has_value());
}

TEST_CASE("History older with empty history is nullopt", "[history]") {
    History h;
    REQUIRE_FALSE(h.older("x").has_value());
}

TEST_CASE("History add after navigation resets cursor", "[history]") {
    History h;
    h.add("a");
    h.add("b");
    (void)h.older("");
    h.add("c");
    REQUIRE_FALSE(h.isNavigating());
    REQUIRE(h.older("") == "c");
}

TEST_CASE("History respects max entries", "[history]") {
    History h(2);
    h.add("1");
    h.add("2");
    h.add("3");
    REQUIRE(h.size() == 2);
    REQUIRE(h.older("") == "3");
    REQUIRE(h.older("") == "2");
    REQUIRE(h.older("") == "2");
}
