#include "optigrab/domain/Errors.hpp"
#include "optigrab/domain/Session.hpp"

#include <catch2/catch_test_macros.hpp>

using optigrab::DriveInfo;
using optigrab::Session;
using optigrab::SessionError;

TEST_CASE("Session requires selected drive", "[session]") {
    Session s;
    REQUIRE_FALSE(s.hasSelectedDrive());
    REQUIRE_THROWS_AS(s.selectedDrive(), SessionError);
}

TEST_CASE("Session selectDrive clears disc", "[session]") {
    Session s;
    DriveInfo d{.path = "/dev/sr0", .model = "X", .index = 0};
    s.selectDrive(d);
    REQUIRE(s.hasSelectedDrive());
    REQUIRE(s.selectedDrive().path == "/dev/sr0");

    optigrab::DiscInfo disc;
    disc.devicePath = "/dev/sr0";
    s.setDisc(disc);
    REQUIRE(s.hasDisc());

    s.selectDrive(DriveInfo{.path = "/dev/sr1", .model = "Y", .index = 1});
    REQUIRE_FALSE(s.hasDisc());
}

namespace {

optigrab::DiscLookup lookupWith(int count) {
    optigrab::DiscLookup l;
    l.discId = "VDjKDudtLNGvkArIWTSGDS3NlR8-";
    for (int i = 0; i < count; ++i) {
        optigrab::Release r;
        r.id = "release-" + std::to_string(i);
        l.releases.push_back(r);
    }
    return l;
}

}  // namespace

TEST_CASE("Session disc lookup selects first release", "[session]") {
    Session s;
    REQUIRE(s.selectedRelease() == nullptr);
    s.setDiscLookup(lookupWith(3));
    REQUIRE(s.discLookup().has_value());
    REQUIRE(s.selectedReleaseIndex() == 0u);
    REQUIRE(s.selectedRelease()->id == "release-0");
    s.selectRelease(2);
    REQUIRE(s.selectedRelease()->id == "release-2");
}

TEST_CASE("Session disc lookup with no releases selects nothing", "[session]") {
    Session s;
    s.setDiscLookup(lookupWith(0));
    REQUIRE(s.discLookup().has_value());
    REQUIRE_FALSE(s.selectedReleaseIndex().has_value());
    REQUIRE(s.selectedRelease() == nullptr);
}

TEST_CASE("Session selectRelease validates state and range", "[session]") {
    Session s;
    REQUIRE_THROWS_AS(s.selectRelease(0), SessionError);
    s.setDiscLookup(lookupWith(2));
    REQUIRE_THROWS_AS(s.selectRelease(2), SessionError);
    REQUIRE(s.selectedReleaseIndex() == 0u);
}

TEST_CASE("Session selectDrive clears disc lookup", "[session]") {
    Session s;
    s.setDiscLookup(lookupWith(1));
    s.selectDrive(DriveInfo{"/dev/sr0", "X", 0});
    REQUIRE_FALSE(s.discLookup().has_value());
    REQUIRE(s.selectedRelease() == nullptr);
}
