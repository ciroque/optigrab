#include "optigrab/domain/Errors.hpp"
#include "optigrab/domain/Session.hpp"
#include "optigrab/services/RipService.hpp"

#include "fakes/FakeAdapters.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace optigrab;
using namespace optigrab::test;

namespace {

class NoCoverProvider : public CoverArtProvider {
public:
    std::optional<CoverArt> fetch(const DiscInfo&, const Session&, Logger*) override {
        return std::nullopt;
    }
    std::string name() const override { return "none"; }
};

RipService makeService() {
    return RipService(std::make_shared<FakeTocReader>(makeTwoTrackDisc()),
                      std::make_shared<FakeExtractor>(), std::make_shared<FakeEncoder>(),
                      std::make_shared<FakeMetadata>(), std::make_shared<NoCoverProvider>(),
                      nullptr);
}

Session baseSession() {
    Session s;
    s.selectDrive(DriveInfo{"/dev/sr0", "X", 0});
    s.setOutputDirectory("/tmp/optigrab-covermissing-test");
    s.setArtist("A");
    s.setAlbum("B");
    s.setFetchCoverArt(true);
    return s;
}

}  // namespace

TEST_CASE("covermissing continue rips without cover", "[covermissing]") {
    auto session = baseSession();
    session.setCoverMissingPolicy(CoverMissingPolicy::Continue);
    auto rip = makeService();
    Logger quiet(std::cerr, LogLevel::Off);
    const auto results = rip.ripTracks(session, {1}, &quiet);
    REQUIRE(results.size() == 1);
    REQUIRE(results[0].success);
}

TEST_CASE("covermissing abort throws before extract", "[covermissing]") {
    auto session = baseSession();
    session.setCoverMissingPolicy(CoverMissingPolicy::Abort);
    auto rip = makeService();
    Logger quiet(std::cerr, LogLevel::Off);
    REQUIRE_THROWS_AS(rip.ripTracks(session, {1}, &quiet), SessionError);
}

TEST_CASE("covermissing ask continues when prompt returns true", "[covermissing]") {
    auto session = baseSession();
    session.setCoverMissingPolicy(CoverMissingPolicy::Ask);
    auto rip = makeService();
    Logger quiet(std::cerr, LogLevel::Off);
    const auto results = rip.ripTracks(session, {1}, &quiet, []() { return true; });
    REQUIRE(results[0].success);
}

TEST_CASE("covermissing ask aborts when prompt returns false", "[covermissing]") {
    auto session = baseSession();
    session.setCoverMissingPolicy(CoverMissingPolicy::Ask);
    auto rip = makeService();
    Logger quiet(std::cerr, LogLevel::Off);
    REQUIRE_THROWS_AS(rip.ripTracks(session, {1}, &quiet, []() { return false; }), SessionError);
}

TEST_CASE("covermissing ask without prompt aborts", "[covermissing]") {
    auto session = baseSession();
    session.setCoverMissingPolicy(CoverMissingPolicy::Ask);
    auto rip = makeService();
    Logger quiet(std::cerr, LogLevel::Off);
    REQUIRE_THROWS_AS(rip.ripTracks(session, {1}, &quiet, {}), SessionError);
}
