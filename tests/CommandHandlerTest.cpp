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
