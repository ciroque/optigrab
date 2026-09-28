#include "optigrab/adapters/manual/ManualMetadataProvider.hpp"
#include "optigrab/domain/Session.hpp"
#include "optigrab/services/RipService.hpp"

#include "fakes/FakeAdapters.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>

using namespace optigrab;
using namespace optigrab::test;

TEST_CASE("RipService rips selected tracks via ports", "[ripservice]") {
    auto toc = std::make_shared<FakeTocReader>(makeTwoTrackDisc());
    auto extractor = std::make_shared<FakeExtractor>();
    auto encoder = std::make_shared<FakeEncoder>();
    auto meta = std::make_shared<FakeMetadata>();
    RipService rip(toc, extractor, encoder, meta);

    Session session;
    session.selectDrive(DriveInfo{"/dev/sr0", "FAKE", 0});
    session.setOutputDirectory("/tmp/optigrab-rip-service-test");
    session.setArtist("Unit");
    session.setAlbum("Test");

    const auto results = rip.ripTracks(session, {1, 2});
    REQUIRE(results.size() == 2);
    REQUIRE(results[0].success);
    REQUIRE(results[1].success);
    REQUIRE(std::filesystem::exists(results[0].outputPath));
    REQUIRE(encoder->lastTags_.title == "Song 2");
    REQUIRE(encoder->lastTags_.album == "Test");
}

TEST_CASE("RipService loadDisc enriches metadata", "[ripservice]") {
    auto toc = std::make_shared<FakeTocReader>(makeTwoTrackDisc());
    auto extractor = std::make_shared<FakeExtractor>();
    auto encoder = std::make_shared<FakeEncoder>();
    auto meta = std::make_shared<FakeMetadata>();
    RipService rip(toc, extractor, encoder, meta);

    Session session;
    session.selectDrive(DriveInfo{"/dev/sr0", "FAKE", 0});
    rip.loadDisc(session);
    REQUIRE(session.hasDisc());
    REQUIRE(session.disc().tracks[0].title == "Song 1");
}

namespace {

// Matches makeTwoTrackDisc(): two audio tracks.
Release mbRelease() {
    Release r;
    r.id = "52d747b1-420b-4d55-ba01-a3ec23d1163d";
    r.title = "MB Album";
    r.artist = "MB Artist";
    r.date = "1999-01-01";
    r.tracks = {ReleaseTrack{1, "First Song", ""}, ReleaseTrack{2, "Second Song", "Guest"}};
    return r;
}

struct MbRig {
    std::shared_ptr<FakeEncoder> encoder = std::make_shared<FakeEncoder>();
    std::shared_ptr<FakeReleaseLookup> releases =
        std::make_shared<FakeReleaseLookup>(std::vector<Release>{mbRelease()});
    RipService rip{std::make_shared<FakeTocReader>(makeTwoTrackDisc()),
                   std::make_shared<FakeExtractor>(),
                   encoder,
                   std::make_shared<ManualMetadataProvider>(),
                   nullptr,
                   nullptr,
                   releases};
    Session session;

    MbRig() { session.selectDrive(DriveInfo{"/dev/sr0", "FAKE", 0}); }
};

}  // namespace

TEST_CASE("RipService loadDisc fills metadata from MusicBrainz once per disc", "[ripservice][mb]") {
    MbRig rig;
    rig.rip.loadDisc(rig.session);
    const auto& disc = rig.session.disc();
    REQUIRE(disc.album == "MB Album");
    REQUIRE(disc.albumArtist == "MB Artist");
    REQUIRE(disc.year == 1999);
    REQUIRE(disc.tracks[0].title == "First Song");
    REQUIRE(disc.tracks[1].artist == "Guest");
    REQUIRE(rig.session.selectedReleaseIndex() == 0u);

    rig.rip.loadDisc(rig.session);  // e.g. list track again
    REQUIRE(rig.releases->calls == 1);
}

TEST_CASE("RipService tags rips with MusicBrainz titles and year", "[ripservice][mb]") {
    MbRig rig;
    rig.session.setOutputDirectory("/tmp/optigrab-rip-service-mb-test");
    rig.session.setFetchCoverArt(false);
    const auto results = rig.rip.ripTracks(rig.session, {2});
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].success);
    const auto& tags = rig.encoder->lastTags_;
    REQUIRE(tags.title == "Second Song");
    REQUIRE(tags.artist == "Guest");
    REQUIRE(tags.album == "MB Album");
    REQUIRE(tags.albumArtist == "MB Artist");
    REQUIRE(tags.year == 1999);
}

TEST_CASE("RipService session overrides beat MusicBrainz but keep guest artists",
          "[ripservice][mb]") {
    MbRig rig;
    rig.session.setArtist("My Artist");
    rig.session.setAlbum("My Album");
    rig.rip.loadDisc(rig.session);
    const auto& disc = rig.session.disc();
    REQUIRE(disc.album == "My Album");
    REQUIRE(disc.albumArtist == "My Artist");
    REQUIRE(disc.tracks[0].artist == "My Artist");
    REQUIRE(disc.tracks[1].artist == "Guest");
    REQUIRE(disc.tracks[0].title == "First Song");
}

TEST_CASE("RipService skips MusicBrainz when disabled", "[ripservice][mb]") {
    MbRig rig;
    rig.session.setMusicBrainzEnabled(false);
    rig.rip.loadDisc(rig.session);
    REQUIRE(rig.releases->calls == 0);
    REQUIRE(rig.session.disc().tracks[0].title == "Track 01");
}

TEST_CASE("RipService falls back to placeholders when lookup fails", "[ripservice][mb]") {
    MbRig rig;
    rig.releases->failWith = "MusicBrainz lookup failed: curl exit 6";
    rig.rip.loadDisc(rig.session);
    REQUIRE(rig.session.disc().tracks[0].title == "Track 01");
    REQUIRE_FALSE(rig.session.discLookup().has_value());
}

TEST_CASE("RipService drops a lookup that belongs to another disc", "[ripservice][mb]") {
    MbRig rig;
    rig.session.setDiscLookup(DiscLookup{"AAAAAAAAAAAAAAAAAAAAAAAAAAA-", {}});
    rig.rip.loadDisc(rig.session);
    REQUIRE(rig.releases->calls == 1);
    REQUIRE(rig.session.disc().album == "MB Album");
}
