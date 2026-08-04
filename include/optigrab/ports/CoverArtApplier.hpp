#pragma once

#include "optigrab/domain/CoverArt.hpp"
#include "optigrab/log/Logger.hpp"

#include <filesystem>
#include <string>

namespace optigrab {

class CoverArtApplier {
public:
    virtual ~CoverArtApplier() = default;

    virtual std::filesystem::path writeSidecar(const std::filesystem::path& albumDir,
                                               const CoverArt& art,
                                               Logger* log = nullptr) = 0;

    virtual void embed(const std::filesystem::path& mp3Path, const CoverArt& art,
                       Logger* log = nullptr) = 0;

    [[nodiscard]] virtual std::string name() const = 0;
};

}  // namespace optigrab
