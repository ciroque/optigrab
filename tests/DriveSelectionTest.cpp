#include "optigrab/cli/CommandHandler.hpp"
#include "optigrab/cli/DriveSelection.hpp"
#include "optigrab/domain/Errors.hpp"
#include "optigrab/services/RipService.hpp"

#include "fakes/FakeAdapters.hpp"

#include <catch2/catch_test_macros.hpp>
#include <sstream>

using namespace optigrab;
using namespace optigrab::test;

namespace {

Context makeCtx(std::shared_ptr<DriveEnumerator> drives, std::ostringstream& out,
                std::ostringstream& err) {
    auto toc = std::make_shared<FakeTocReader>(makeTwoTrackDisc());
    auto ripper = std::make_shared<RipService>(toc, std::make_shared<FakeExtractor>(),
                                               std::make_shared<FakeEncoder>(),
                                               std::make_shared<FakeMetadata>());
    auto rebuild = [toc](ExtractorKind, EncoderKind) {
        return std::make_shared<RipService>(toc, std::make_shared<FakeExtractor>(),
                                            std::make_shared<FakeEncoder>(),
                                            std::make_shared<FakeMetadata>());
    };
    return Context(std::move(drives), std::move(ripper), rebuild, out, err);
}

}  // namespace

TEST_CASE("tryAutoSelectSingleDrive selects sole drive", "[drive-selection]") {
    std::ostringstream out, err;
    auto drives = std::make_shared<FakeDriveEnumerator>(
        std::vector<DriveInfo>{DriveInfo{"/dev/sr0", "ONLY", 0}});
    auto ctx = makeCtx(drives, out, err);

    REQUIRE(tryAutoSelectSingleDrive(ctx, true));
    REQUIRE(ctx.session.hasSelectedDrive());
    REQUIRE(ctx.session.selectedDrive().path == "/dev/sr0");
    REQUIRE(out.str().find("[auto]") != std::string::npos);
}

TEST_CASE("tryAutoSelectSingleDrive leaves multi-drive alone", "[drive-selection]") {
    std::ostringstream out, err;
    auto drives = std::make_shared<FakeDriveEnumerator>(std::vector<DriveInfo>{
        DriveInfo{"/dev/sr0", "A", 0},
        DriveInfo{"/dev/sr1", "B", 1},
    });
    auto ctx = makeCtx(drives, out, err);

    REQUIRE_FALSE(tryAutoSelectSingleDrive(ctx, true));
    REQUIRE_FALSE(ctx.session.hasSelectedDrive());
}

TEST_CASE("ensureDriveSelected auto-selects single drive", "[drive-selection]") {
    std::ostringstream out, err;
    auto drives = std::make_shared<FakeDriveEnumerator>(
        std::vector<DriveInfo>{DriveInfo{"D:", "DVD", 0}});
    auto ctx = makeCtx(drives, out, err);

    REQUIRE_NOTHROW(ensureDriveSelected(ctx));
    REQUIRE(ctx.session.selectedDrive().path == "D:");
}

TEST_CASE("ensureDriveSelected errors when multiple drives", "[drive-selection]") {
    std::ostringstream out, err;
    auto drives = std::make_shared<FakeDriveEnumerator>(std::vector<DriveInfo>{
        DriveInfo{"/dev/sr0", "A", 0},
        DriveInfo{"/dev/sr1", "B", 1},
    });
    auto ctx = makeCtx(drives, out, err);

    REQUIRE_THROWS_AS(ensureDriveSelected(ctx), SessionError);
}

TEST_CASE("list track auto-selects when only one drive", "[drive-selection][cli]") {
    std::ostringstream out, err;
    auto drives = std::make_shared<FakeDriveEnumerator>(
        std::vector<DriveInfo>{DriveInfo{"/dev/sr0", "ONLY", 0}});
    auto ctx = makeCtx(drives, out, err);
    auto handler = makeDefaultCommandHandler();

    handler.execute(ctx, "list track");
    REQUIRE(ctx.session.hasSelectedDrive());
    REQUIRE(out.str().find("Song") != std::string::npos);
}
