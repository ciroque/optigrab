#pragma once

#include "optigrab/domain/Types.hpp"

#include <filesystem>
#include <functional>
#include <string>

namespace optigrab {

class AudioExtractor {
public:
    using ProgressFn = std::function<void(const std::string& message)>;

    virtual ~AudioExtractor() = default;

    // Extract a single audio track to a WAV path (PCM 16-bit LE stereo 44.1kHz).
    virtual void extractTrack(const std::string& devicePath,
                              const TrackInfo& track,
                              const std::filesystem::path& outputWav,
                              ProgressFn progress = {}) = 0;

    [[nodiscard]] virtual std::string name() const = 0;
};

}  // namespace optigrab
