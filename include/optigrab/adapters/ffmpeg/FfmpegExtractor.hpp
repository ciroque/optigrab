#pragma once

#include "optigrab/ports/AudioExtractor.hpp"

#include <string>

namespace optigrab {

class FfmpegExtractor : public AudioExtractor {
public:
    explicit FfmpegExtractor(std::string binary = "ffmpeg");

    void extractTrack(const std::string& devicePath,
                      const TrackInfo& track,
                      const std::filesystem::path& outputWav,
                      ProgressFn progress = {}) override;
    [[nodiscard]] std::string name() const override { return "ffmpeg"; }

private:
    std::string binary_;
};

}  // namespace optigrab
