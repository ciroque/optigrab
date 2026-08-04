#pragma once

#include "optigrab/domain/Types.hpp"

#include <optional>
#include <string>

namespace optigrab {

// MusicBrainz Disc ID from audio-track TOC (LBA offsets).
// Returns nullopt if the disc has no usable audio tracks.
[[nodiscard]] std::optional<std::string> computeMusicBrainzDiscId(const DiscInfo& disc);

}  // namespace optigrab
