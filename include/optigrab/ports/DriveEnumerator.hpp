#pragma once

#include "optigrab/domain/Types.hpp"

#include <vector>

namespace optigrab {

class DriveEnumerator {
public:
    virtual ~DriveEnumerator() = default;
    [[nodiscard]] virtual std::vector<DriveInfo> listDrives() = 0;
};

}  // namespace optigrab
