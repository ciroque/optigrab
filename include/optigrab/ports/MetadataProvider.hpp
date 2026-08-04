#pragma once

#include "optigrab/domain/Types.hpp"

#include <string>

namespace optigrab {

// Enrich a disc/track list with titles (and optional album/artist).
class MetadataProvider {
public:
    virtual ~MetadataProvider() = default;
    virtual void enrich(DiscInfo& disc) = 0;
    [[nodiscard]] virtual std::string name() const = 0;
};

}  // namespace optigrab
