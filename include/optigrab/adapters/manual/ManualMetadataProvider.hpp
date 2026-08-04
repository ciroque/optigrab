#pragma once

#include "optigrab/ports/MetadataProvider.hpp"

namespace optigrab {

// Fills missing titles with "Track NN". Respects existing CD-TEXT titles.
class ManualMetadataProvider : public MetadataProvider {
public:
    void enrich(DiscInfo& disc) override;
    [[nodiscard]] std::string name() const override { return "manual"; }
};

}  // namespace optigrab
