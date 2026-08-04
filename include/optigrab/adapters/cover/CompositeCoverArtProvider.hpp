#pragma once

#include "optigrab/ports/CoverArtProvider.hpp"

#include <memory>
#include <vector>

namespace optigrab {

// Tries providers in order; first hit wins (typically local, then network).
class CompositeCoverArtProvider : public CoverArtProvider {
public:
    explicit CompositeCoverArtProvider(std::vector<std::shared_ptr<CoverArtProvider>> providers);

    [[nodiscard]] std::optional<CoverArt> fetch(const DiscInfo& disc,
                                                const Session& session) override;
    [[nodiscard]] std::string name() const override { return "composite"; }

private:
    std::vector<std::shared_ptr<CoverArtProvider>> providers_;
};

}  // namespace optigrab
