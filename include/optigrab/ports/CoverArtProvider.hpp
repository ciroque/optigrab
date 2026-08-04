#pragma once

#include "optigrab/domain/CoverArt.hpp"
#include "optigrab/domain/Session.hpp"
#include "optigrab/domain/Types.hpp"

#include <optional>
#include <string>

namespace optigrab {

class CoverArtProvider {
public:
    virtual ~CoverArtProvider() = default;

    // Fail soft: return nullopt if no art (network error, not found, etc.).
    [[nodiscard]] virtual std::optional<CoverArt> fetch(const DiscInfo& disc,
                                                        const Session& session) = 0;

    [[nodiscard]] virtual std::string name() const = 0;
};

}  // namespace optigrab
