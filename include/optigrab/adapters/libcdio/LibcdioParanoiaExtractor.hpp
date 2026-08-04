#pragma once

#include "optigrab/ports/AudioExtractor.hpp"

namespace optigrab {

class LibcdioParanoiaExtractor : public AudioExtractor {
public:
    void extractTrack(const std::string& devicePath,
                      const TrackInfo& track,
                      const std::filesystem::path& outputWav,
                      ProgressFn progress = {}) override;
    [[nodiscard]] std::string name() const override { return "libcdio_paranoia"; }
};

}  // namespace optigrab
