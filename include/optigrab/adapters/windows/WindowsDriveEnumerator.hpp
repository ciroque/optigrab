#pragma once

#include "optigrab/ports/DriveEnumerator.hpp"

namespace optigrab {

class WindowsDriveEnumerator : public DriveEnumerator {
public:
    [[nodiscard]] std::vector<DriveInfo> listDrives() override;
};

}  // namespace optigrab
