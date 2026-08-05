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

// Log file under logPathDir: "<Artist> - <Album>.log" (sanitized).
[[nodiscard]] std::filesystem::path buildLogFilePath(const std::filesystem::path& logPathDir,
                                                     const std::string& artist,
                                                     const std::string& album);

// Prefer albumArtist / session-style fields from tags.
[[nodiscard]] std::filesystem::path buildLogFilePath(const std::filesystem::path& logPathDir,
                                                     const Tags& tags);

}  // namespace optigrab
