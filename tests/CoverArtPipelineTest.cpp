#include "optigrab/domain/Session.hpp"
#include "optigrab/ports/CoverArtApplier.hpp"
#include "optigrab/ports/CoverArtProvider.hpp"
#include "optigrab/services/RipService.hpp"

#include "fakes/FakeAdapters.hpp"

#include <catch2/catch_test_macros.hpp>
#include <filesystem>
#include <fstream>

using namespace optigrab;
using namespace optigrab::test;

namespace {

CoverArt tinyJpeg() {
    CoverArt art;
    art.bytes = {0xFF, 0xD8, 0xFF, 0xD9, 0x00, 0x01, 0x02, 0x03};
    art.mimeType = "image/jpeg";
    art.source = "fake";
    return art;
}

class FakeCoverProvider : public CoverArtProvider {
public:
    explicit FakeCoverProvider(std::optional<CoverArt> art) : art_(std::move(art)) {}
    std::optional<CoverArt> fetch(const DiscInfo&, const Session&, Logger* log) override {
        if (log) {
            if (art_) {
                log->debug("[fake] providing cover");
            } else {
                log->debug("[fake] no cover by design");
            }
        }
        return art_;
    }
    std::string name() const override { return "fake-cover"; }

private:
    std::optional<CoverArt> art_;
};

class FakeCoverApplier : public CoverArtApplier {
public:
    std::filesystem::path writeSidecar(const std::filesystem::path& albumDir, const CoverArt& art,
                                       Logger*) override {
        ++sidecarCalls;
        lastAlbumDir = albumDir;
        std::filesystem::create_directories(albumDir);
        const auto p = albumDir / "cover.jpg";
        std::ofstream out(p, std::ios::binary);
        out.write(reinterpret_cast<const char*>(art.bytes.data()),
                  static_cast<std::streamsize>(art.bytes.size()));
        return p;
    }
    void embed(const std::filesystem::path& mp3Path, const CoverArt&, Logger*) override {
        embedded.push_back(mp3Path);
        std::ofstream out(mp3Path.string() + ".embedded", std::ios::binary);
        out << "embedded";
    }
    std::string name() const override { return "fake-applier"; }

    int sidecarCalls{0};
    std::filesystem::path lastAlbumDir;
    std::vector<std::filesystem::path> embedded;
};

}  // namespace

TEST_CASE("RipService downloads cover then rips then embeds", "[cover]") {
    auto toc = std::make_shared<FakeTocReader>(makeTwoTrackDisc());
    auto coverProv = std::make_shared<FakeCoverProvider>(tinyJpeg());
    auto coverApp = std::make_shared<FakeCoverApplier>();

    RipService rip(toc, std::make_shared<FakeExtractor>(), std::make_shared<FakeEncoder>(),
                   std::make_shared<FakeMetadata>(), coverProv, coverApp);

    Session session;
    session.selectDrive(DriveInfo{"/dev/sr0", "FAKE", 0});
    const auto outRoot = std::filesystem::temp_directory_path() / "optigrab-cover-test";
    std::filesystem::remove_all(outRoot);
    session.setOutputDirectory(outRoot.string());
    session.setArtist("Unit");
    session.setAlbum("CoverTest");

    Logger log(std::cerr, LogLevel::Off);
    const auto results = rip.ripTracks(session, {1, 2}, &log);
    REQUIRE(results.size() == 2);
    REQUIRE(results[0].success);
    REQUIRE(results[1].success);
    REQUIRE(coverApp->sidecarCalls == 1);
    REQUIRE(coverApp->embedded.size() == 2);
    REQUIRE(std::filesystem::exists(coverApp->lastAlbumDir / "cover.jpg"));
    std::filesystem::remove_all(outRoot);
}

TEST_CASE("RipService skips cover when disabled", "[cover]") {
    auto coverApp = std::make_shared<FakeCoverApplier>();
    RipService rip(std::make_shared<FakeTocReader>(makeTwoTrackDisc()),
                   std::make_shared<FakeExtractor>(), std::make_shared<FakeEncoder>(),
                   std::make_shared<FakeMetadata>(), std::make_shared<FakeCoverProvider>(tinyJpeg()),
                   coverApp);

    Session session;
    session.selectDrive(DriveInfo{"/dev/sr0", "FAKE", 0});
    session.setFetchCoverArt(false);
    session.setOutputDirectory(
        (std::filesystem::temp_directory_path() / "optigrab-cover-off").string());
    session.setArtist("A");
    session.setAlbum("B");

    REQUIRE(rip.ripTracks(session, {1})[0].success);
    REQUIRE(coverApp->sidecarCalls == 0);
    REQUIRE(coverApp->embedded.empty());
}

TEST_CASE("RipService continues when cover missing", "[cover]") {
    auto coverApp = std::make_shared<FakeCoverApplier>();
    RipService rip(std::make_shared<FakeTocReader>(makeTwoTrackDisc()),
                   std::make_shared<FakeExtractor>(), std::make_shared<FakeEncoder>(),
                   std::make_shared<FakeMetadata>(),
                   std::make_shared<FakeCoverProvider>(std::nullopt), coverApp);

    Session session;
    session.selectDrive(DriveInfo{"/dev/sr0", "FAKE", 0});
    session.setOutputDirectory(
        (std::filesystem::temp_directory_path() / "optigrab-cover-none").string());
    session.setArtist("A");
    session.setAlbum("B");

    REQUIRE(rip.ripTracks(session, {1})[0].success);
    REQUIRE(coverApp->sidecarCalls == 0);
}
