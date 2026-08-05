#pragma once

#include "optigrab/ports/DriveEjector.hpp"

namespace optigrab {

class LinuxDriveEjector : public DriveEjector {
public:
    void eject(const std::string& devicePath) override;
};

}  // namespace optigrab
