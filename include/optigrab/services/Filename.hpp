#pragma once

#include "optigrab/domain/Types.hpp"

#include <filesystem>
#include <string>

namespace optigrab {

[[nodiscard]] std::string sanitizeFilename(std::string name);

// Builds: <out>/<Artist> - <Album>/<NN> <Title>.mp3
[[nodiscard]] std::filesystem::path buildTrackPath(const std::filesystem::path& outputDir,
                                                   const Tags& tags);

}  // namespace optigrab
