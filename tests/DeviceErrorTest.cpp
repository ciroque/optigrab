#include "optigrab/util/DeviceError.hpp"

#include <catch2/catch_test_macros.hpp>
#include <cerrno>

using optigrab::describeDeviceFailure;

TEST_CASE("describeDeviceFailure includes path and operation", "[device-error]") {
    const auto msg = describeDeviceFailure("/dev/sr0", EIO, "Open optical device");
    REQUIRE(msg.find("/dev/sr0") != std::string::npos);
    REQUIRE(msg.find("Open optical device") != std::string::npos);
}

#ifndef _WIN32
TEST_CASE("describeDeviceFailure permission hint on Linux", "[device-error]") {
    const auto msg = describeDeviceFailure("/dev/sr0", EACCES, "Open optical device");
    REQUIRE(msg.find("hint:") != std::string::npos);
    REQUIRE(msg.find("optical") != std::string::npos);
}

TEST_CASE("describeDeviceFailure busy hint", "[device-error]") {
    const auto msg = describeDeviceFailure("/dev/sr0", EBUSY, "Open optical device");
    REQUIRE(msg.find("busy") != std::string::npos);
}
#endif
