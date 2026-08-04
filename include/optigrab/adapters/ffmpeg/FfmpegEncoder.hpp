#pragma once

#include "optigrab/ports/AudioEncoder.hpp"

#include <string>

namespace optigrab {

class FfmpegEncoder : public AudioEncoder {
public:
    explicit FfmpegEncoder(std::string binary = "ffmpeg");

    void encode(const std::filesystem::path& inputWav,
                const std::filesystem::path& outputMp3,
                const Tags& tags,
                QualityPreset quality,
                ProgressFn progress = {}) override;
    [[nodiscard]] std::string name() const override { return "ffmpeg"; }

private:
    std::string binary_;
};

}  // namespace optigrab
