#include "optigrab/domain/Release.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace optigrab;

namespace {

DiscInfo discWithDataTrack() {
    DiscInfo d;
    d.tracks.push_back(TrackInfo{1, 0, 100, true, "Track 01", ""});
    d.tracks.push_back(TrackInfo{2, 100, 200, true, "Track 02", ""});
    d.tracks.push_back(TrackInfo{3, 400, 900, false, "", ""});  // enhanced-CD data track
    return d;
}

Release twoTrackRelease() {
    Release r;
    r.id = "52d747b1-420b-4d55-ba01-a3ec23d1163d";
    r.title = "Duets";
    r.artist = "Alice";
    r.date = "2004-03";
    r.tracks = {ReleaseTrack{1, "Opening", ""}, ReleaseTrack{2, "Closer", "Alice feat. Bob"}};
    return r;
}

}  // namespace

TEST_CASE("applyRelease copies album, artists, titles, and year", "[release]") {
    auto disc = discWithDataTrack();
    REQUIRE(applyRelease(disc, twoTrackRelease()));
    REQUIRE(disc.album == "Duets");
    REQUIRE(disc.albumArtist == "Alice");
    REQUIRE(disc.year == 2004);
    REQUIRE(disc.tracks[0].title == "Opening");
    REQUIRE(disc.tracks[0].artist == "Alice");  // no track credit → release artist
    REQUIRE(disc.tracks[1].title == "Closer");
    REQUIRE(disc.tracks[1].artist == "Alice feat. Bob");
    REQUIRE(disc.tracks[2].title.empty());  // data track untouched
}

TEST_CASE("applyRelease refuses a release with a different track count", "[release]") {
    auto disc = discWithDataTrack();
    auto r = twoTrackRelease();
    r.tracks.pop_back();
    REQUIRE_FALSE(applyRelease(disc, r));
    REQUIRE_FALSE(disc.album.has_value());
    REQUIRE(disc.tracks[0].title == "Track 01");
}
