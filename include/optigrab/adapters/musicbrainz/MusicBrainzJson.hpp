#pragma once

#include "optigrab/domain/Release.hpp"

#include <string>
#include <vector>

namespace optigrab {

// Parse a MusicBrainz /ws/2/discid/<id>?inc=artist-credits+recordings&fmt=json body.
// Each release keeps only the tracks of the medium carrying discId.
// Throws OptigrabError if the body is not valid JSON.
[[nodiscard]] std::vector<Release> parseDiscIdReleases(const std::string& json,
                                                       const std::string& discId);

}  // namespace optigrab
