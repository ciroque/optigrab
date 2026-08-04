#include "optigrab/adapters/cover/CompositeCoverArtProvider.hpp"

namespace optigrab {

CompositeCoverArtProvider::CompositeCoverArtProvider(
    std::vector<std::shared_ptr<CoverArtProvider>> providers)
    : providers_(std::move(providers)) {}

std::optional<CoverArt> CompositeCoverArtProvider::fetch(const DiscInfo& disc,
                                                         const Session& session) {
    for (const auto& p : providers_) {
        if (!p) {
            continue;
        }
        if (auto art = p->fetch(disc, session)) {
            return art;
        }
    }
    return std::nullopt;
}

}  // namespace optigrab
