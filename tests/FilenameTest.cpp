#include "optigrab/services/Filename.hpp"

#include <catch2/catch_test_macros.hpp>

using optigrab::buildTrackPath;
using optigrab::sanitizeFilename;
using optigrab::Tags;

TEST_CASE("sanitizeFilename strips banned characters", "[filename]") {
    REQUIRE(sanitizeFilename("A/B:C*") == "A_B_C_");
    REQUIRE(sanitizeFilename("") == "Unknown");
}

TEST_CASE("buildTrackPath uses artist album and track title", "[filename]") {
    Tags tags;
    tags.artist = "Artist";
    tags.albumArtist = "Artist";
    tags.album = "Album";
    tags.title = "Hello";
    tags.trackNumber = 3;
    const auto p = buildTrackPath("/music", tags);
    REQUIRE(p.filename() == "03 Hello.mp3");
    REQUIRE(p.parent_path().filename() == "Artist - Album");
}
