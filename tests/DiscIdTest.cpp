#include "optigrab/domain/DiscId.hpp"

#include <catch2/catch_test_macros.hpp>

using optigrab::computeMusicBrainzDiscId;
using optigrab::computeMusicBrainzDiscIdFromOffsets;
using optigrab::DiscInfo;
using optigrab::musicBrainzSubmitUrl;
using optigrab::musicBrainzToc;
using optigrab::TrackInfo;

TEST_CASE("MusicBrainz disc ID matches known libdiscid vector", "[discid]") {
    // From MusicBrainz API for discid I5l9cCSFccLKFEKS.7wqSZAorPU-
    // offsets = track starts in frames; sectors = lead-out frame
    const std::vector<std::uint32_t> trackStarts = {
        150,    22767,  41887,  58317,  72102,  91375,
        104652, 115380, 132165, 143932, 159870, 174597,
    };
    const std::uint32_t leadOut = 267257;
    const auto id = computeMusicBrainzDiscIdFromOffsets(1, 12, leadOut, trackStarts);
    REQUIRE(id.has_value());
    REQUIRE(*id == "I5l9cCSFccLKFEKS.7wqSZAorPU-");
    REQUIRE(id->size() == 28);
    REQUIRE(id->back() == '-');  // Base64 pad becomes '-'
}

TEST_CASE("MusicBrainz disc ID from DiscInfo LBA matches frames", "[discid]") {
    // Same disc: frame = LBA + 150, length in sectors between starts
    const std::vector<std::int64_t> startsLba = {
        0, 22617, 41737, 58167, 71952, 91225, 104502, 115230, 132015, 143782, 159720, 174447,
    };
    // leadOut frame 267257 → LBA 267107; last track start LBA 174447 → sectors = 267107-174447
    DiscInfo d;
    for (std::size_t i = 0; i < startsLba.size(); ++i) {
        TrackInfo t;
        t.number = static_cast<int>(i + 1);
        t.startLba = startsLba[i];
        t.audio = true;
        if (i + 1 < startsLba.size()) {
            t.sectors = startsLba[i + 1] - startsLba[i];
        } else {
            t.sectors = 267107 - startsLba[i];  // lead-out LBA - last start
        }
        d.tracks.push_back(t);
    }

    const auto id = computeMusicBrainzDiscId(d);
    REQUIRE(id.has_value());
    REQUIRE(*id == "I5l9cCSFccLKFEKS.7wqSZAorPU-");
}

TEST_CASE("computeMusicBrainzDiscId nullopt for empty disc", "[discid]") {
    DiscInfo d;
    REQUIRE_FALSE(computeMusicBrainzDiscId(d).has_value());
}

TEST_CASE("disc ID is valid MB charset", "[discid]") {
    DiscInfo d;
    d.tracks.push_back(TrackInfo{1, 0, 1000, true, "", ""});
    d.tracks.push_back(TrackInfo{2, 1000, 2000, true, "", ""});
    const auto id = computeMusicBrainzDiscId(d);
    REQUIRE(id.has_value());
    REQUIRE(id->size() == 28);
    for (char c : *id) {
        const bool ok = (c >= 'A' && c <= 'Z') || (c >= 'a' && c <= 'z') ||
                        (c >= '0' && c <= '9') || c == '.' || c == '_' || c == '-';
        REQUIRE(ok);
    }
}

namespace {

// Same 12-track disc as the libdiscid vector above.
DiscInfo knownTwelveTrackDisc() {
    const std::vector<std::int64_t> startsLba = {
        0, 22617, 41737, 58167, 71952, 91225, 104502, 115230, 132015, 143782, 159720, 174447,
    };
    DiscInfo d;
    for (std::size_t i = 0; i < startsLba.size(); ++i) {
        TrackInfo t;
        t.number = static_cast<int>(i + 1);
        t.startLba = startsLba[i];
        t.sectors = (i + 1 < startsLba.size() ? startsLba[i + 1] : 267107) - startsLba[i];
        d.tracks.push_back(t);
    }
    return d;
}

}  // namespace

TEST_CASE("MusicBrainz TOC string lists first, last, lead-out, then offsets", "[discid]") {
    const auto toc = musicBrainzToc(knownTwelveTrackDisc());
    REQUIRE(toc.has_value());
    REQUIRE(*toc ==
            "1 12 267257 150 22767 41887 58317 72102 91375 104652 115380 132165 143932 "
            "159870 174597");
}

TEST_CASE("MusicBrainz TOC ignores trailing data track", "[discid]") {
    auto d = knownTwelveTrackDisc();
    d.tracks.push_back(TrackInfo{13, 280000, 5000, false, "", ""});
    REQUIRE(musicBrainzToc(d) == musicBrainzToc(knownTwelveTrackDisc()));
    REQUIRE(computeMusicBrainzDiscId(d) == "I5l9cCSFccLKFEKS.7wqSZAorPU-");
}

TEST_CASE("MusicBrainz submit URL carries id, track count, and TOC", "[discid]") {
    const auto url = musicBrainzSubmitUrl(knownTwelveTrackDisc());
    REQUIRE(url.has_value());
    REQUIRE(*url ==
            "https://musicbrainz.org/cdtoc/attach?id=I5l9cCSFccLKFEKS.7wqSZAorPU-&tracks=12"
            "&toc=1+12+267257+150+22767+41887+58317+72102+91375+104652+115380+132165+143932+"
            "159870+174597");
}

TEST_CASE("MusicBrainz TOC and submit URL nullopt for empty disc", "[discid]") {
    REQUIRE_FALSE(musicBrainzToc(DiscInfo{}).has_value());
    REQUIRE_FALSE(musicBrainzSubmitUrl(DiscInfo{}).has_value());
}
