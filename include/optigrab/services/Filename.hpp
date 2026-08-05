#pragma once

#include "optigrab/domain/Types.hpp"

#include <filesystem>
#include <string>

namespace optigrab {

[[nodiscard]] std::string sanitizeFilename(std::string name);

// Builds track path under outputDir according to folder layout:
//   nested  out/Artist/Album/<NN> Title.mp3
//   joined  out/Artist - Album/<NN> Title.mp3
//   album   out/Album/<NN> Title.mp3
[[nodiscard]] std::filesystem::path buildTrackPath(const std::filesystem::path& outputDir,
                                                   const Tags& tags,
                                                   FolderLayout layout = FolderLayout::Nested);

}  // namespace optigrab
