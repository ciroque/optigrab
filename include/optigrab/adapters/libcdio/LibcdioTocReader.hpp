#pragma once

#include "optigrab/ports/TocReader.hpp"

namespace optigrab {

class LibcdioTocReader : public TocReader {
public:
    [[nodiscard]] DiscInfo readToc(const std::string& devicePath) override;
};

}  // namespace optigrab
