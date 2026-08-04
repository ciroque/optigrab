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
