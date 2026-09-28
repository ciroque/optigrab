#pragma once

#include "optigrab/domain/Types.hpp"

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace optigrab {

// MusicBrainz Disc ID from audio-track TOC (LBA → CD frames).
// Compatible with libdiscid (ASCII-hex SHA-1 + modified Base64).
[[nodiscard]] std::optional<std::string> computeMusicBrainzDiscId(const DiscInfo& disc);

// MusicBrainz TOC string "first last leadout off1 off2 ..." (CD frames), as used by
// the ws/2 ?toc= parameter and the disc submission page.
[[nodiscard]] std::optional<std::string> musicBrainzToc(const DiscInfo& disc);

// Page for attaching this disc ID to a MusicBrainz release (for discs not yet known).
[[nodiscard]] std::optional<std::string> musicBrainzSubmitUrl(const DiscInfo& disc);

// Direct libdiscid-style inputs (frames, not LBA): leadOut + per-track starts for first..last.
[[nodiscard]] std::optional<std::string> computeMusicBrainzDiscIdFromOffsets(
    int first, int last, std::uint32_t leadOutFrames,
    const std::vector<std::uint32_t>& trackStartFrames);

}  // namespace optigrab
