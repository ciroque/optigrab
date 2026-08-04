#include "optigrab/adapters/cover/CompositeCoverArtProvider.hpp"

namespace optigrab {

CompositeCoverArtProvider::CompositeCoverArtProvider(
    std::vector<std::shared_ptr<CoverArtProvider>> providers)
    : providers_(std::move(providers)) {}

std::optional<CoverArt> CompositeCoverArtProvider::fetch(const DiscInfo& disc,
                                                         const Session& session, LogFn log) {
    if (providers_.empty()) {
        if (log) {
            log("  [composite] no cover providers configured");
        }
        return std::nullopt;
    }

    for (const auto& p : providers_) {
        if (!p) {
            continue;
        }
        if (log) {
            log("  trying provider: " + p->name());
        }
        try {
            if (auto art = p->fetch(disc, session, log)) {
                if (log) {
                    log("  provider " + p->name() + " succeeded (" + art->source + ")");
                }
                return art;
            }
            if (log) {
                log("  provider " + p->name() + " returned no cover");
            }
        } catch (const std::exception& ex) {
            if (log) {
                log("  provider " + p->name() + " threw: " + ex.what());
            }
        }
    }
    if (log) {
        log("  all cover providers exhausted — no art");
    }
    return std::nullopt;
}

}  // namespace optigrab
