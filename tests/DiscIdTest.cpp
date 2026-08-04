#include "optigrab/domain/DiscId.hpp"

#include <catch2/catch_test_macros.hpp>

using optigrab::computeMusicBrainzDiscId;
using optigrab::DiscInfo;
using optigrab::TrackInfo;

TEST_CASE("computeMusicBrainzDiscId returns 28-char id for simple TOC", "[discid]") {
    DiscInfo d;
    d.tracks.push_back(TrackInfo{1, 0, 1000, true, "", ""});
    d.tracks.push_back(TrackInfo{2, 1000, 2000, true, "", ""});
    const auto id = computeMusicBrainzDiscId(d);
    REQUIRE(id.has_value());
    REQUIRE(id->size() == 28);
}

TEST_CASE("computeMusicBrainzDiscId nullopt for empty disc", "[discid]") {
    DiscInfo d;
    REQUIRE_FALSE(computeMusicBrainzDiscId(d).has_value());
}
