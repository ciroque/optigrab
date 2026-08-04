#pragma once

#include "optigrab/ports/TocReader.hpp"

namespace optigrab {

// Reads CD-DA TOC via Windows SPTI (IOCTL_CDROM_READ_TOC).
class WindowsTocReader : public TocReader {
public:
    [[nodiscard]] DiscInfo readToc(const std::string& devicePath) override;
};

}  // namespace optigrab
