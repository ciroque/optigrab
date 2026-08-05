#pragma once

#include "optigrab/ports/DriveEnumerator.hpp"

namespace optigrab {

class MacDriveEnumerator : public DriveEnumerator {
public:
    [[nodiscard]] std::vector<DriveInfo> listDrives() override;
};

}  // namespace optigrab
