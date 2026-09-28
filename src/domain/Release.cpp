#include "optigrab/domain/Release.hpp"

#include <cctype>

namespace optigrab {

std::optional<int> Release::year() const {
    if (date.size() < 4) {
        return std::nullopt;
    }
    int y = 0;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto c = static_cast<unsigned char>(date[i]);
        if (!std::isdigit(c)) {
            return std::nullopt;
        }
        y = y * 10 + (c - '0');
    }
    return y;
}

}  // namespace optigrab
