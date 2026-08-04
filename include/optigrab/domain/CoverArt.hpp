#pragma once

#include <cstdint>
#include <filesystem>
#include <string>
#include <vector>

namespace optigrab {

struct CoverArt {
    std::vector<std::uint8_t> bytes;
    std::string mimeType;  // image/jpeg, image/png
    std::string source;    // "local", "coverartarchive", etc.
};

[[nodiscard]] inline bool isJpeg(const std::vector<std::uint8_t>& b) {
    return b.size() >= 3 && b[0] == 0xFF && b[1] == 0xD8 && b[2] == 0xFF;
}

[[nodiscard]] inline bool isPng(const std::vector<std::uint8_t>& b) {
    static const std::uint8_t sig[] = {0x89, 'P', 'N', 'G', '\r', '\n', 0x1a, '\n'};
    if (b.size() < 8) {
        return false;
    }
    for (int i = 0; i < 8; ++i) {
        if (b[static_cast<std::size_t>(i)] != sig[i]) {
            return false;
        }
    }
    return true;
}

[[nodiscard]] inline std::string guessMime(const std::vector<std::uint8_t>& b) {
    if (isJpeg(b)) {
        return "image/jpeg";
    }
    if (isPng(b)) {
        return "image/png";
    }
    return "application/octet-stream";
}

[[nodiscard]] inline std::string sidecarExtension(const CoverArt& art) {
    if (art.mimeType == "image/png" || isPng(art.bytes)) {
        return ".png";
    }
    return ".jpg";
}

}  // namespace optigrab
