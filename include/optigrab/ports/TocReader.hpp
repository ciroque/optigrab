#pragma once

#include "optigrab/domain/Types.hpp"

#include <string>

namespace optigrab {

class TocReader {
public:
    virtual ~TocReader() = default;
    [[nodiscard]] virtual DiscInfo readToc(const std::string& devicePath) = 0;
};

}  // namespace optigrab
