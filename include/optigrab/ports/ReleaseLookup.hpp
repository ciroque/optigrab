#pragma once

#include "optigrab/domain/Release.hpp"
#include "optigrab/log/Logger.hpp"

#include <string>

namespace optigrab {

// Resolve a MusicBrainz Disc ID to the releases it belongs to.
class ReleaseLookup {
public:
    virtual ~ReleaseLookup() = default;

    // Unknown disc → DiscLookup with no releases.
    // Network/service failures throw LookupError.
    [[nodiscard]] virtual DiscLookup lookup(const std::string& discId, Logger* log = nullptr) = 0;

    [[nodiscard]] virtual std::string name() const = 0;
};

}  // namespace optigrab
