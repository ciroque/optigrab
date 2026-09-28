#include "optigrab/adapters/musicbrainz/Curl.hpp"

#include <catch2/catch_test_macros.hpp>

using optigrab::musicBrainzUserAgent;

TEST_CASE("MusicBrainz User-Agent carries the version", "[curl]") {
    REQUIRE(musicBrainzUserAgent("0.3.1") ==
            "optigrab/0.3.1 (https://github.com/ciroque/optigrab)");
    REQUIRE(musicBrainzUserAgent("v1.0.0-rc1") ==
            "optigrab/1.0.0-rc1 (https://github.com/ciroque/optigrab)");
    REQUIRE(musicBrainzUserAgent("") == "optigrab/dev (https://github.com/ciroque/optigrab)");
}

TEST_CASE("MusicBrainz User-Agent defaults to the build version", "[curl]") {
    REQUIRE(musicBrainzUserAgent() == musicBrainzUserAgent(OPTIGRAB_VERSION_STRING));
}
