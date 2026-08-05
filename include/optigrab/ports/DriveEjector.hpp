#pragma once

#include <string>

namespace optigrab {

class DriveEjector {
public:
    virtual ~DriveEjector() = default;
    virtual void eject(const std::string& devicePath) = 0;
};

}  // namespace optigrab
