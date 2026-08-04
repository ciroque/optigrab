#pragma once

#include "optigrab/ports/MetadataProvider.hpp"

namespace optigrab {

// Leaves titles empty (RipService will use "Track N").
class NullMetadataProvider : public MetadataProvider {
public:
    void enrich(DiscInfo&) override {}
    [[nodiscard]] std::string name() const override { return "null"; }
};

}  // namespace optigrab
