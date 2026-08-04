#include "optigrab/log/Logger.hpp"

#include <catch2/catch_test_macros.hpp>
#include <sstream>
#include <vector>

using optigrab::Logger;
using optigrab::LogLevel;
using optigrab::parseLogLevel;
using optigrab::toString;

TEST_CASE("parseLogLevel accepts common names", "[log]") {
    REQUIRE(parseLogLevel("debug") == LogLevel::Debug);
    REQUIRE(parseLogLevel("WARN") == LogLevel::Warn);
    REQUIRE(parseLogLevel("error") == LogLevel::Error);
    REQUIRE(parseLogLevel("fatal") == LogLevel::Fatal);
    REQUIRE(parseLogLevel("off") == LogLevel::Off);
    REQUIRE_FALSE(parseLogLevel("nope").has_value());
}

TEST_CASE("Logger filters below min level", "[log]") {
    std::vector<std::pair<LogLevel, std::string>> seen;
    Logger log(std::cerr, LogLevel::Warn);
    log.setSink([&](LogLevel level, std::string_view msg) {
        seen.emplace_back(level, std::string(msg));
    });

    log.debug("hidden");
    log.info("hidden");
    log.warn("visible-w");
    log.error("visible-e");

    REQUIRE(seen.size() == 2);
    REQUIRE(seen[0].first == LogLevel::Warn);
    REQUIRE(seen[1].first == LogLevel::Error);
}

TEST_CASE("Logger callback respects level", "[log]") {
    int count = 0;
    Logger log(std::cerr, LogLevel::Debug);
    log.setSink([&](LogLevel, std::string_view) { ++count; });
    auto cb = log.callback(LogLevel::Debug);
    cb("a");
    cb("b");
    REQUIRE(count == 2);
    REQUIRE(std::string(toString(LogLevel::Info)) == "INFO");
}
