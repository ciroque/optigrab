#include "optigrab/adapters/musicbrainz/MusicBrainzJson.hpp"
#include "optigrab/domain/Errors.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace optigrab;

namespace {

constexpr const char* kDiscId = "VDjKDudtLNGvkArIWTSGDS3NlR8-";

// Trimmed shape of a real /ws/2/discid response with inc=artist-credits+recordings.
constexpr const char* kTwoDiscSet = R"({
  "id": "VDjKDudtLNGvkArIWTSGDS3NlR8-",
  "releases": [
    {
      "id": "52d747b1-420b-4d55-ba01-a3ec23d1163d",
      "title": "Piece of Mind",
      "date": "1983-05-16",
      "country": "GB",
      "barcode": null,
      "artist-credit": [
        {"name": "Iron Maiden", "joinphrase": "", "artist": {"id": "ca891d65-d9b0-4258-89f7-e6ba29d83767", "name": "Iron Maiden"}}
      ],
      "cover-art-archive": {"front": true, "count": 5, "artwork": true},
      "media": [
        {
          "position": 1,
          "format-id": "9712d52a-4509-3d4b-a1a2-67c88c643e31",
          "id": "4417209c-77b4-31fc-add6-229154d0a4c7",
          "discs": [{"id": "SomeOtherDiscId______________"}],
          "tracks": [{"position": 1, "title": "Wrong Disc", "artist-credit": []}]
        },
        {
          "position": 2,
          "discs": [{"id": "VDjKDudtLNGvkArIWTSGDS3NlR8-"}],
          "tracks": [
            {"position": 1, "number": "1", "title": "Where Eagles Dare",
             "artist-credit": [{"name": "Iron Maiden", "joinphrase": ""}]},
            {"position": 2, "number": "2", "title": "Revelations",
             "artist-credit": [{"name": "Iron Maiden", "joinphrase": ""}]}
          ]
        }
      ],
      "release-events": [
        {"area": {"id": "489ce91b-6658-3307-9877-795b68554c98", "name": "United Kingdom"}}
      ]
    }
  ]
})";

}  // namespace

TEST_CASE("discid parse reads release fields", "[mb-json]") {
    const auto releases = parseDiscIdReleases(kTwoDiscSet, kDiscId);
    REQUIRE(releases.size() == 1);
    const auto& r = releases[0];
    REQUIRE(r.id == "52d747b1-420b-4d55-ba01-a3ec23d1163d");
    REQUIRE(r.title == "Piece of Mind");
    REQUIRE(r.artist == "Iron Maiden");
    REQUIRE(r.date == "1983-05-16");
    REQUIRE(r.year() == 1983);
    REQUIRE(r.country == "GB");
    REQUIRE(r.barcode.empty());  // null → empty
    REQUIRE(r.hasFrontCover);
    REQUIRE(r.mediumCount == 2);
}

TEST_CASE("discid parse picks the medium carrying the disc ID", "[mb-json]") {
    const auto releases = parseDiscIdReleases(kTwoDiscSet, kDiscId);
    REQUIRE(releases.size() == 1);
    const auto& r = releases[0];
    REQUIRE(r.mediumPosition == 2);
    REQUIRE(r.tracks.size() == 2);
    REQUIRE(r.tracks[0].position == 1);
    REQUIRE(r.tracks[0].title == "Where Eagles Dare");
    REQUIRE(r.tracks[0].artist == "Iron Maiden");
    REQUIRE(r.tracks[1].title == "Revelations");
}

TEST_CASE("discid parse flattens multi-artist credits with join phrases", "[mb-json]") {
    const std::string json = R"({"releases": [{
      "id": "11111111-2222-3333-4444-555555555555",
      "title": "Duets",
      "artist-credit": [
        {"name": "Alice", "joinphrase": " & "},
        {"name": "Bob", "joinphrase": ""}
      ],
      "media": [{"position": 1, "discs": [], "tracks": [
        {"position": 1, "title": "Song", "artist-credit": [
          {"name": "Alice", "joinphrase": " feat. "}, {"name": "Carol", "joinphrase": ""}
        ]}
      ]}]
    }]})";
    const auto releases = parseDiscIdReleases(json, kDiscId);
    REQUIRE(releases.size() == 1);
    REQUIRE(releases[0].artist == "Alice & Bob");
    // Single medium with no disc match is still used.
    REQUIRE(releases[0].tracks.size() == 1);
    REQUIRE(releases[0].tracks[0].artist == "Alice feat. Carol");
    REQUIRE_FALSE(releases[0].hasFrontCover);
    REQUIRE_FALSE(releases[0].year());
}

TEST_CASE("discid parse leaves tracks empty when medium is ambiguous", "[mb-json]") {
    const std::string json = R"({"releases": [{
      "id": "11111111-2222-3333-4444-555555555555",
      "title": "Box",
      "media": [
        {"position": 1, "discs": [], "tracks": [{"position": 1, "title": "A"}]},
        {"position": 2, "discs": [], "tracks": [{"position": 1, "title": "B"}]}
      ]
    }]})";
    const auto releases = parseDiscIdReleases(json, kDiscId);
    REQUIRE(releases.size() == 1);
    REQUIRE(releases[0].mediumPosition == 0);
    REQUIRE(releases[0].tracks.empty());
}

TEST_CASE("discid parse returns nothing for CD stubs and unknown discs", "[mb-json]") {
    REQUIRE(parseDiscIdReleases(R"({"error": "Not Found"})", kDiscId).empty());
    REQUIRE(parseDiscIdReleases(R"({"id": "x", "title": "stub", "tracks": []})", kDiscId).empty());
    REQUIRE(parseDiscIdReleases(R"([])", kDiscId).empty());
}

TEST_CASE("discid parse throws on malformed JSON", "[mb-json]") {
    REQUIRE_THROWS_AS(parseDiscIdReleases("<html>503</html>", kDiscId), OptigrabError);
}

TEST_CASE("release year requires four leading digits", "[mb-json]") {
    Release r;
    r.date = "2001";
    REQUIRE(r.year() == 2001);
    r.date = "19";
    REQUIRE_FALSE(r.year());
    r.date = "unknown";
    REQUIRE_FALSE(r.year());
}
