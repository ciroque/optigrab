#include "optigrab/cli/Tokenizer.hpp"
#include "optigrab/domain/Errors.hpp"

#include <catch2/catch_test_macros.hpp>

using optigrab::tokenize;
using optigrab::ParseError;

TEST_CASE("tokenize splits on whitespace", "[tokenizer]") {
    const auto t = tokenize("list drive");
    REQUIRE(t.size() == 2);
    REQUIRE(t[0] == "list");
    REQUIRE(t[1] == "drive");
}

TEST_CASE("tokenize respects double quotes", "[tokenizer]") {
    const auto t = tokenize(R"(set artist "The Band")");
    REQUIRE(t.size() == 3);
    REQUIRE(t[0] == "set");
    REQUIRE(t[1] == "artist");
    REQUIRE(t[2] == "The Band");
}

TEST_CASE("tokenize rejects unbalanced quotes", "[tokenizer]") {
    REQUIRE_THROWS_AS(tokenize("set album \"Oops"), ParseError);
}

TEST_CASE("tokenize returns empty for blank input", "[tokenizer]") {
    REQUIRE(tokenize("   ").empty());
}
