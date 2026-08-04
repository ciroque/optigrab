#include "fakes/FakeAdapters.hpp"

#include <catch2/catch_test_macros.hpp>

using namespace optigrab::test;

TEST_CASE("FakeDriveEnumerator returns configured drives", "[fakes]") {
    FakeDriveEnumerator e({optigrab::DriveInfo{"/dev/sr0", "M", 0}});
    const auto d = e.listDrives();
    REQUIRE(d.size() == 1);
    REQUIRE(d[0].path == "/dev/sr0");
}
