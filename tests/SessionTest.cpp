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
