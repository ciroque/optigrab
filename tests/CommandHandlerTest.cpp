#include "optigrab/cli/CommandHandler.hpp"
#include "optigrab/cli/Context.hpp"
#include "optigrab/services/RipService.hpp"

#include "fakes/FakeAdapters.hpp"

#include <catch2/catch_test_macros.hpp>
#include <sstream>

using namespace optigrab;
using namespace optigrab::test;

namespace {

Context makeTestContext(std::ostringstream& out, std::ostringstream& err) {
    auto drives = std::make_shared<FakeDriveEnumerator>(std::vector<DriveInfo>{
        DriveInfo{"/dev/sr0", "FAKE-DRIVE", 0},
        DriveInfo{"/dev/sr1", "OTHER", 1},
    });
    auto toc = std::make_shared<FakeTocReader>(makeTwoTrackDisc());
    auto extractor = std::make_shared<FakeExtractor>();
    auto encoder = std::make_shared<FakeEncoder>();
    auto meta = std::make_shared<FakeMetadata>();
    auto ripper = std::make_shared<RipService>(toc, extractor, encoder, meta);

    auto rebuild = [toc, meta](ExtractorKind, EncoderKind) {
        return std::make_shared<RipService>(toc, std::make_shared<FakeExtractor>(),
                                            std::make_shared<FakeEncoder>(), meta);
    };

    return Context(drives, ripper, rebuild, out, err);
}

}  // namespace

TEST_CASE("list drive prints enumerated drives", "[cli]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "list drive");
    REQUIRE(out.str().find("/dev/sr0") != std::string::npos);
    REQUIRE(out.str().find("FAKE-DRIVE") != std::string::npos);
}

TEST_CASE("select drive by index", "[cli]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "select drive 1");
    REQUIRE(ctx.session.hasSelectedDrive());
    REQUIRE(ctx.session.selectedDrive().path == "/dev/sr1");
}

TEST_CASE("list track requires selection when multiple drives", "[cli]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);  // two fake drives
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "list track");
    REQUIRE(err.str().find("No drive selected") != std::string::npos);
    REQUIRE(err.str().find("Multiple drives") != std::string::npos);
}

TEST_CASE("set artist and album", "[cli]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, R"(set artist "The Band")");
    handler.execute(ctx, R"(set album "Live At Budokan")");
    REQUIRE(ctx.session.artist() == "The Band");
    REQUIRE(ctx.session.album() == "Live At Budokan");
}

TEST_CASE("detail context dumps session without drive", "[cli]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, R"(set artist "The Band")");
    handler.execute(ctx, R"(set album "Live")");
    handler.execute(ctx, "set out /tmp/music");
    handler.execute(ctx, "set folderlayout joined");
    handler.execute(ctx, "detail context");
    REQUIRE(ctx.exitCode == 0);
    const auto text = out.str();
    REQUIRE(text.find("Key") != std::string::npos);
    REQUIRE(text.find("Value") != std::string::npos);
    REQUIRE(text.find("Choices") != std::string::npos);
    REQUIRE(text.find("Via") != std::string::npos);
    REQUIRE(text.find("artist") != std::string::npos);
    REQUIRE(text.find("The Band") != std::string::npos);
    REQUIRE(text.find("album") != std::string::npos);
    REQUIRE(text.find("Live") != std::string::npos);
    REQUIRE(text.find("out") != std::string::npos);
    REQUIRE(text.find("/tmp/music") != std::string::npos);
    REQUIRE(text.find("folderlayout") != std::string::npos);
    REQUIRE(text.find("joined") != std::string::npos);
    REQUIRE(text.find("nested|joined|album") != std::string::npos);
    REQUIRE(text.find("V0|V2|192|256|320") != std::string::npos);
    REQUIRE(text.find("ask|continue|abort") != std::string::npos);
    REQUIRE(text.find("trace|debug|info|warn|error|fatal|off") != std::string::npos);
    REQUIRE(text.find("set out / --out") != std::string::npos);
    REQUIRE(text.find("set folderlayout / --folder-layout") != std::string::npos);
    REQUIRE(text.find("set artist / --artist") != std::string::npos);
    REQUIRE(text.find("select drive / --drive") != std::string::npos);
}

TEST_CASE("exit sets shouldExit", "[cli]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "exit");
    REQUIRE(ctx.shouldExit);
}

TEST_CASE("unknown verb noun reports error", "[cli]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "list sandwich");
    REQUIRE(err.str().find("unknown") != std::string::npos);
}

TEST_CASE("rip track all with fakes", "[cli]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "select drive 0");
    handler.execute(ctx, "set out /tmp/optigrab-test-out");
    handler.execute(ctx, R"(set artist "Fake")");
    handler.execute(ctx, R"(set album "Disc")");
    handler.execute(ctx, "rip track all");
    REQUIRE(out.str().find("succeeded") != std::string::npos);
    REQUIRE(out.str().find("failed") != std::string::npos);
}

namespace {

Release fakeRelease() {
    Release r;
    r.id = "52d747b1-420b-4d55-ba01-a3ec23d1163d";
    r.title = "Piece of Mind";
    r.artist = "Iron Maiden";
    r.date = "1983-05-16";
    r.country = "GB";
    r.hasFrontCover = true;
    r.mediumPosition = 1;
    r.mediumCount = 1;
    r.tracks = {ReleaseTrack{1, "Where Eagles Dare", "Iron Maiden"},
                ReleaseTrack{2, "Revelations", "Iron Maiden"}};
    return r;
}

}  // namespace

TEST_CASE("lookup disc prints releases and tracks for the selected disc", "[cli][lookup]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto releases = std::make_shared<FakeReleaseLookup>(std::vector<Release>{fakeRelease()});
    ctx.releases = releases;
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "select drive 0");
    handler.execute(ctx, "lookup disc");
    REQUIRE(ctx.exitCode == 0);
    REQUIRE(releases->calls == 1);
    const auto text = out.str();
    REQUIRE(text.find("Disc ID  : " + releases->lastDiscId) != std::string::npos);
    REQUIRE(text.find("Iron Maiden - Piece of Mind") != std::string::npos);
    REQUIRE(text.find("1983-05-16") != std::string::npos);
    REQUIRE(text.find("Where Eagles Dare") != std::string::npos);
    REQUIRE(text.find("Revelations") != std::string::npos);
}

TEST_CASE("lookup disc with explicit disc ID needs no drive", "[cli][lookup]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);  // two drives, none selected
    auto releases = std::make_shared<FakeReleaseLookup>(std::vector<Release>{fakeRelease()});
    ctx.releases = releases;
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "lookup disc VDjKDudtLNGvkArIWTSGDS3NlR8-");
    REQUIRE(ctx.exitCode == 0);
    REQUIRE(releases->lastDiscId == "VDjKDudtLNGvkArIWTSGDS3NlR8-");
    REQUIRE(out.str().find("Piece of Mind") != std::string::npos);
}

TEST_CASE("lookup disc offers submit link when disc is unknown", "[cli][lookup]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    ctx.releases = std::make_shared<FakeReleaseLookup>();
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "select drive 0");
    handler.execute(ctx, "lookup disc");
    REQUIRE(ctx.exitCode == 0);
    REQUIRE(out.str().find("Not in MusicBrainz yet") != std::string::npos);
    REQUIRE(out.str().find("https://musicbrainz.org/cdtoc/attach?id=") != std::string::npos);
}

TEST_CASE("lookup disc rejects malformed disc IDs", "[cli][lookup]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto releases = std::make_shared<FakeReleaseLookup>();
    ctx.releases = releases;
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "lookup disc not-a-disc-id");
    REQUIRE(ctx.exitCode != 0);
    REQUIRE(releases->calls == 0);
    REQUIRE(err.str().find("Not a MusicBrainz disc ID") != std::string::npos);
}

TEST_CASE("lookup disc reports service failures", "[cli][lookup]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto releases = std::make_shared<FakeReleaseLookup>();
    releases->failWith = "MusicBrainz lookup failed: curl exit 22 (HTTP 503)";
    ctx.releases = releases;
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "select drive 0");
    handler.execute(ctx, "lookup disc");
    REQUIRE(ctx.exitCode != 0);
    REQUIRE(err.str().find("HTTP 503") != std::string::npos);
}

TEST_CASE("lookup disc keeps result in session and select release switches", "[cli][lookup]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    auto second = fakeRelease();
    second.id = "11111111-2222-3333-4444-555555555555";
    second.date = "1998";
    second.country = "XE";
    ctx.releases =
        std::make_shared<FakeReleaseLookup>(std::vector<Release>{fakeRelease(), second});
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "select drive 0");
    handler.execute(ctx, "lookup disc");
    REQUIRE(ctx.exitCode == 0);
    REQUIRE(out.str().find("select release <#>") != std::string::npos);
    REQUIRE(ctx.session.discLookup().has_value());
    REQUIRE(ctx.session.selectedReleaseIndex() == 0u);

    handler.execute(ctx, "select release 1");
    REQUIRE(ctx.exitCode == 0);
    REQUIRE(ctx.session.selectedRelease()->id == second.id);
    REQUIRE(out.str().find("Release 1 is now the selected release") != std::string::npos);
}

TEST_CASE("lookup disc with explicit disc ID does not touch session", "[cli][lookup]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    ctx.releases = std::make_shared<FakeReleaseLookup>(std::vector<Release>{fakeRelease()});
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "lookup disc VDjKDudtLNGvkArIWTSGDS3NlR8-");
    REQUIRE(ctx.exitCode == 0);
    REQUIRE_FALSE(ctx.session.discLookup().has_value());
}

TEST_CASE("select release requires a lookup and a valid index", "[cli][lookup]") {
    std::ostringstream out, err;
    auto ctx = makeTestContext(out, err);
    ctx.releases = std::make_shared<FakeReleaseLookup>(std::vector<Release>{fakeRelease()});
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "select release 0");
    REQUIRE(err.str().find("lookup disc") != std::string::npos);

    ctx.exitCode = 0;
    handler.execute(ctx, "select drive 0");
    handler.execute(ctx, "lookup disc");
    handler.execute(ctx, "select release 5");
    REQUIRE(err.str().find("out of range") != std::string::npos);
    ctx.exitCode = 0;
    handler.execute(ctx, "select release x");
    REQUIRE(err.str().find("must be a number") != std::string::npos);
}

namespace {

// Ripper wired to a MusicBrainz fake, as in the real composition root.
std::unique_ptr<Context> makeMbContext(std::ostringstream& out, std::ostringstream& err,
                                       std::shared_ptr<FakeReleaseLookup> releases) {
    auto drives = std::make_shared<FakeDriveEnumerator>(
        std::vector<DriveInfo>{DriveInfo{"/dev/sr0", "FAKE-DRIVE", 0}});
    auto toc = std::make_shared<FakeTocReader>(makeTwoTrackDisc());
    auto meta = std::make_shared<FakeMetadata>();
    auto make = [toc, meta, releases](ExtractorKind, EncoderKind) {
        return std::make_shared<RipService>(toc, std::make_shared<FakeExtractor>(),
                                            std::make_shared<FakeEncoder>(), meta, nullptr,
                                            nullptr, releases);
    };
    auto ctx = std::make_unique<Context>(
        drives, make(ExtractorKind::Ffmpeg, EncoderKind::Ffmpeg), make, out, err);
    ctx->releases = releases;
    ctx->session.selectDrive(DriveInfo{"/dev/sr0", "FAKE-DRIVE", 0});
    return ctx;
}

}  // namespace

TEST_CASE("lookup disc on a fresh disc queries MusicBrainz once", "[cli][lookup]") {
    std::ostringstream out, err;
    auto releases = std::make_shared<FakeReleaseLookup>(std::vector<Release>{fakeRelease()});
    auto ctxp = makeMbContext(out, err, releases);
    auto& ctx = *ctxp;
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "lookup disc");
    REQUIRE(ctx.exitCode == 0);
    REQUIRE(releases->calls == 1);
    REQUIRE(ctx.session.disc().tracks[0].title == "Where Eagles Dare");

    handler.execute(ctx, "lookup disc");  // explicit refresh
    REQUIRE(releases->calls == 2);
}

TEST_CASE("select release re-applies titles to the loaded disc", "[cli][lookup]") {
    std::ostringstream out, err;
    auto other = fakeRelease();
    other.id = "11111111-2222-3333-4444-555555555555";
    other.title = "Piece of Mind (Remaster)";
    other.tracks[0].title = "Where Eagles Dare (2015 Remaster)";
    auto releases =
        std::make_shared<FakeReleaseLookup>(std::vector<Release>{fakeRelease(), other});
    auto ctxp = makeMbContext(out, err, releases);
    auto& ctx = *ctxp;
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "list track");
    REQUIRE(out.str().find("Where Eagles Dare") != std::string::npos);

    handler.execute(ctx, "select release 1");
    REQUIRE(ctx.exitCode == 0);
    REQUIRE(ctx.session.disc().album == "Piece of Mind (Remaster)");
    REQUIRE(ctx.session.disc().tracks[0].title == "Where Eagles Dare (2015 Remaster)");
    REQUIRE(releases->calls == 1);
}

TEST_CASE("set musicbrainz off stops automatic lookups", "[cli][lookup]") {
    std::ostringstream out, err;
    auto releases = std::make_shared<FakeReleaseLookup>(std::vector<Release>{fakeRelease()});
    auto ctxp = makeMbContext(out, err, releases);
    auto& ctx = *ctxp;
    auto handler = makeDefaultCommandHandler();
    handler.execute(ctx, "set musicbrainz off");
    handler.execute(ctx, "list track");
    REQUIRE(ctx.exitCode == 0);
    REQUIRE(releases->calls == 0);
    REQUIRE_FALSE(ctx.session.musicBrainzEnabled());

    handler.execute(ctx, "lookup disc");  // explicit lookup still allowed
    REQUIRE(releases->calls == 1);
    REQUIRE(ctx.session.disc().tracks[0].title == "Where Eagles Dare");
}
