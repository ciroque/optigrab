#include "optigrab/adapters/manual/ManualMetadataProvider.hpp"

#include <iomanip>
#include <sstream>

namespace optigrab {

void ManualMetadataProvider::enrich(DiscInfo& disc) {
    for (auto& t : disc.tracks) {
        if (!t.audio) {
            continue;
        }
        if (t.title.empty()) {
            std::ostringstream oss;
            oss << "Track " << std::setw(2) << std::setfill('0') << t.number;
            t.title = oss.str();
        }
    }
}

}  // namespace optigrab
