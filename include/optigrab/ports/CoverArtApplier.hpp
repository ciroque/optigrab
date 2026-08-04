#pragma once

#include "optigrab/domain/CoverArt.hpp"

#include <filesystem>
#include <functional>
#include <string>
#include <vector>

namespace optigrab {

class CoverArtApplier {
public:
    using LogFn = std::function<void(const std::string&)>;

    virtual ~CoverArtApplier() = default;

    // Write cover.jpg/png once into albumDir. Returns path written.
    virtual std::filesystem::path writeSidecar(const std::filesystem::path& albumDir,
                                               const CoverArt& art,
                                               LogFn log = {}) = 0;

    // Embed APIC into an existing MP3 (rewrites file in place via temp).
    virtual void embed(const std::filesystem::path& mp3Path, const CoverArt& art,
                       LogFn log = {}) = 0;

    [[nodiscard]] virtual std::string name() const = 0;
};

}  // namespace optigrab
