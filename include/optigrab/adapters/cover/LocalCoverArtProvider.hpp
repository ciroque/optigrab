#pragma once

#include "optigrab/ports/CoverArtProvider.hpp"

namespace optigrab {

class LocalCoverArtProvider : public CoverArtProvider {
public:
    [[nodiscard]] std::optional<CoverArt> fetch(const DiscInfo& disc, const Session& session,
                                                LogFn log = {}) override;
    [[nodiscard]] std::string name() const override { return "local"; }
};

}  // namespace optigrab
