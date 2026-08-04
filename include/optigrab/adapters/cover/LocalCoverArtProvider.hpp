#pragma once

#include "optigrab/ports/CoverArtProvider.hpp"

namespace optigrab {

// Uses Session::coverPath() if set (user-supplied image file).
class LocalCoverArtProvider : public CoverArtProvider {
public:
    [[nodiscard]] std::optional<CoverArt> fetch(const DiscInfo& disc,
                                                const Session& session) override;
    [[nodiscard]] std::string name() const override { return "local"; }
};

}  // namespace optigrab
