#include "optigrab/services/Filename.hpp"

#include <iomanip>
#include <sstream>

namespace optigrab {

std::string sanitizeFilename(std::string name) {
    if (name.empty()) {
        return "Unknown";
    }

    const std::string banned = "/\\:*?\"<>|";
    for (char& c : name) {
        if (static_cast<unsigned char>(c) < 32 || banned.find(c) != std::string::npos) {
            c = '_';
        }
    }

    // Collapse consecutive spaces/underscores lightly: trim edges.
    while (!name.empty() && (name.front() == ' ' || name.front() == '.')) {
        name.erase(name.begin());
    }
    while (!name.empty() && (name.back() == ' ' || name.back() == '.')) {
        name.pop_back();
    }

    if (name.empty()) {
        return "Unknown";
    }
    return name;
}

std::filesystem::path buildTrackPath(const std::filesystem::path& outputDir, const Tags& tags,
                                     FolderLayout layout) {
    const std::string artist =
        sanitizeFilename(tags.albumArtist.empty() ? tags.artist : tags.albumArtist);
    const std::string album = sanitizeFilename(tags.album.empty() ? "Unknown Album" : tags.album);
    const std::string title = sanitizeFilename(
        tags.title.empty() ? ("Track " + std::to_string(tags.trackNumber)) : tags.title);

    std::ostringstream nn;
    nn << std::setw(2) << std::setfill('0') << tags.trackNumber;
    const std::string file = nn.str() + " " + title + ".mp3";

    switch (layout) {
    case FolderLayout::Nested:
        return outputDir / artist / album / file;
    case FolderLayout::Joined:
        return outputDir / (artist + " - " + album) / file;
    case FolderLayout::Album:
        return outputDir / album / file;
    }
    return outputDir / artist / album / file;
}

}  // namespace optigrab
