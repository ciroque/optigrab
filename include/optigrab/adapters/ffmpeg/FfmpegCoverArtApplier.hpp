#pragma once

#include "optigrab/ports/CoverArtApplier.hpp"

#include <string>

namespace optigrab {

class FfmpegCoverArtApplier : public CoverArtApplier {
public:
    explicit FfmpegCoverArtApplier(std::string binary = "ffmpeg");

    std::filesystem::path writeSidecar(const std::filesystem::path& albumDir, const CoverArt& art,
                                       LogFn log = {}) override;

    void embed(const std::filesystem::path& mp3Path, const CoverArt& art, LogFn log = {}) override;

    [[nodiscard]] std::string name() const override { return "ffmpeg"; }

private:
    std::string binary_;
};

}  // namespace optigrab
