#include "optigrab/cli/Args.hpp"
#include "optigrab/domain/Errors.hpp"

#include <catch2/catch_test_macros.hpp>

using optigrab::FolderLayout;
using optigrab::parseLaunchArgs;
using optigrab::ParseError;
using optigrab::QualityPreset;

TEST_CASE("parseLaunchArgs empty is interactive", "[args]") {
    const auto a = parseLaunchArgs({});
    REQUIRE(a.interactive);
    REQUIRE(a.command.empty());
}

TEST_CASE("parseLaunchArgs one-shot command", "[args]") {
    const auto a = parseLaunchArgs({"list", "drive"});
    REQUIRE_FALSE(a.interactive);
    REQUIRE(a.command.size() == 2);
    REQUIRE(a.command[0] == "list");
    REQUIRE(a.command[1] == "drive");
}

TEST_CASE("parseLaunchArgs options then command", "[args]") {
    const auto a = parseLaunchArgs({"--drive", "0", "--out", "/tmp/music", "--artist", "Band",
                                    "--album", "Live", "--quality", "V0", "rip", "track", "all"});
    REQUIRE_FALSE(a.interactive);
    REQUIRE(a.drive == "0");
    REQUIRE(a.outDir == "/tmp/music");
    REQUIRE(a.artist == "Band");
    REQUIRE(a.album == "Live");
    REQUIRE(a.quality == QualityPreset::V0);
    REQUIRE(a.command == std::vector<std::string>{"rip", "track", "all"});
}

TEST_CASE("parseLaunchArgs help and version", "[args]") {
    REQUIRE(parseLaunchArgs({"--help"}).showHelp);
    REQUIRE(parseLaunchArgs({"-h"}).showHelp);
    REQUIRE(parseLaunchArgs({"--version"}).showVersion);
}

TEST_CASE("parseLaunchArgs rejects unknown flag", "[args]") {
    REQUIRE_THROWS_AS(parseLaunchArgs({"--nope"}), ParseError);
}

TEST_CASE("parseLaunchArgs double dash ends options", "[args]") {
    const auto a = parseLaunchArgs({"--drive", "1", "--", "list", "track"});
    REQUIRE(a.drive == "1");
    REQUIRE(a.command == std::vector<std::string>{"list", "track"});
}

TEST_CASE("parseLaunchArgs folder-layout", "[args]") {
    REQUIRE(parseLaunchArgs({"--folder-layout", "nested"}).folderLayout == FolderLayout::Nested);
    REQUIRE(parseLaunchArgs({"--folder-layout", "joined"}).folderLayout == FolderLayout::Joined);
    REQUIRE(parseLaunchArgs({"--folderlayout", "album"}).folderLayout == FolderLayout::Album);
    REQUIRE_THROWS_AS(parseLaunchArgs({"--folder-layout", "flat"}), ParseError);
}

TEST_CASE("parseLaunchArgs log-file", "[args]") {
    const auto a = parseLaunchArgs({"--log-file", "/tmp/optigrab.log", "list", "drive"});
    REQUIRE(a.logFile == "/tmp/optigrab.log");
    REQUIRE(a.command == std::vector<std::string>{"list", "drive"});
    REQUIRE(parseLaunchArgs({"--logfile", "x.log"}).logFile == "x.log");
}
