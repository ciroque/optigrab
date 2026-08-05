#include "optigrab/services/Filename.hpp"

#include <catch2/catch_test_macros.hpp>

using optigrab::buildTrackPath;
using optigrab::FolderLayout;
using optigrab::sanitizeFilename;
using optigrab::Tags;

namespace {

Tags sampleTags() {
    Tags tags;
    tags.artist = "Artist";
    tags.albumArtist = "Artist";
    tags.album = "Album";
    tags.title = "Hello";
    tags.trackNumber = 3;
    return tags;
}

}  // namespace

TEST_CASE("sanitizeFilename strips banned characters", "[filename]") {
    REQUIRE(sanitizeFilename("A/B:C*") == "A_B_C_");
    REQUIRE(sanitizeFilename("") == "Unknown");
}

TEST_CASE("buildTrackPath nested is Artist/Album/track", "[filename]") {
    const auto p = buildTrackPath("/music", sampleTags(), FolderLayout::Nested);
    REQUIRE(p.filename() == "03 Hello.mp3");
    REQUIRE(p.parent_path().filename() == "Album");
    REQUIRE(p.parent_path().parent_path().filename() == "Artist");
    REQUIRE(p == std::filesystem::path("/music") / "Artist" / "Album" / "03 Hello.mp3");
}

TEST_CASE("buildTrackPath joined is Artist - Album/track", "[filename]") {
    const auto p = buildTrackPath("/music", sampleTags(), FolderLayout::Joined);
    REQUIRE(p.filename() == "03 Hello.mp3");
    REQUIRE(p.parent_path().filename() == "Artist - Album");
}

TEST_CASE("buildTrackPath album is Album/track only", "[filename]") {
    const auto p = buildTrackPath("/music", sampleTags(), FolderLayout::Album);
    REQUIRE(p.filename() == "03 Hello.mp3");
    REQUIRE(p.parent_path().filename() == "Album");
    REQUIRE(p.parent_path().parent_path().filename() == "music");
}

TEST_CASE("buildTrackPath defaults to nested", "[filename]") {
    const auto p = buildTrackPath("/music", sampleTags());
    REQUIRE(p.parent_path().filename() == "Album");
    REQUIRE(p.parent_path().parent_path().filename() == "Artist");
}

TEST_CASE("buildTrackPath prefers albumArtist for folder name", "[filename]") {
    Tags tags = sampleTags();
    tags.artist = "Track Artist";
    tags.albumArtist = "Album Artist";
    const auto p = buildTrackPath("/music", tags, FolderLayout::Nested);
    REQUIRE(p.parent_path().parent_path().filename() == "Album Artist");
}

TEST_CASE("buildLogFilePath uses Artist - Album.log", "[filename]") {
    using optigrab::buildLogFilePath;
    const auto p = buildLogFilePath("/var/log/optigrab", "Iron Maiden", "Piece of Mind");
    REQUIRE(p.filename() == "Iron Maiden - Piece of Mind.log");
    REQUIRE(p.parent_path() == "/var/log/optigrab");

    Tags tags = sampleTags();
    tags.albumArtist = "The Band";
    tags.album = "Live";
    REQUIRE(buildLogFilePath("/logs", tags).filename() == "The Band - Live.log");
}

TEST_CASE("buildLogFilePath falls back for empty names", "[filename]") {
    using optigrab::buildLogFilePath;
    REQUIRE(buildLogFilePath("/l", "", "").filename() == "Unknown Artist - Unknown Album.log");
}
