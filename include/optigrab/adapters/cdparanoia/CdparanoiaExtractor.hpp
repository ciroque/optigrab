#pragma once

#include "optigrab/ports/AudioExtractor.hpp"

#include <string>

namespace optigrab {

class CdparanoiaExtractor : public AudioExtractor {
public:
    explicit CdparanoiaExtractor(std::string binary = "cdparanoia");

    void extractTrack(const std::string& devicePath,
                      const TrackInfo& track,
                      const std::filesystem::path& outputWav,
                      ProgressFn progress = {}) override;
    [[nodiscard]] std::string name() const override { return "cdparanoia"; }

private:
    std::string binary_;
};

}  // namespace optigrab
