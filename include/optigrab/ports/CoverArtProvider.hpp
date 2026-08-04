#pragma once

#include "optigrab/domain/CoverArt.hpp"
#include "optigrab/domain/Session.hpp"
#include "optigrab/domain/Types.hpp"

#include <functional>
#include <optional>
#include <string>

namespace optigrab {

class CoverArtProvider {
public:
    using LogFn = std::function<void(const std::string&)>;

    virtual ~CoverArtProvider() = default;

    // Fail soft: return nullopt if no art. Use log for step-by-step diagnostics.
    [[nodiscard]] virtual std::optional<CoverArt> fetch(const DiscInfo& disc,
                                                        const Session& session,
                                                        LogFn log = {}) = 0;

    [[nodiscard]] virtual std::string name() const = 0;
};

}  // namespace optigrab
