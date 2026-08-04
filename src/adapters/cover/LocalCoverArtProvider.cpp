#include "optigrab/adapters/cover/LocalCoverArtProvider.hpp"

#include <fstream>

namespace optigrab {

std::optional<CoverArt> LocalCoverArtProvider::fetch(const DiscInfo&, const Session& session) {
    if (!session.coverPath()) {
        return std::nullopt;
    }
    const auto path = *session.coverPath();
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        return std::nullopt;
    }
    CoverArt art;
    art.bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    if (art.bytes.empty()) {
        return std::nullopt;
    }
    art.mimeType = guessMime(art.bytes);
    art.source = "local:" + path;
    return art;
}

}  // namespace optigrab
