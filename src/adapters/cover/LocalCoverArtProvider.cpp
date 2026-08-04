#include "optigrab/adapters/cover/LocalCoverArtProvider.hpp"

#include <filesystem>
#include <fstream>

namespace optigrab {

std::optional<CoverArt> LocalCoverArtProvider::fetch(const DiscInfo&, const Session& session,
                                                     Logger* log) {
    if (!session.coverPath()) {
        if (log) {
            log->debug("[local] no set cover path (skipping local file)");
        }
        return std::nullopt;
    }
    const auto path = *session.coverPath();
    if (log) {
        log->debug("[local] trying " + path);
    }
    if (!std::filesystem::exists(path)) {
        if (log) {
            log->warn("[local] file does not exist: " + path);
        }
        return std::nullopt;
    }
    std::ifstream in(path, std::ios::binary);
    if (!in) {
        if (log) {
            log->warn("[local] cannot open file for reading: " + path);
        }
        return std::nullopt;
    }
    CoverArt art;
    art.bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    if (art.bytes.empty()) {
        if (log) {
            log->warn("[local] file is empty: " + path);
        }
        return std::nullopt;
    }
    if (!isJpeg(art.bytes) && !isPng(art.bytes)) {
        if (log) {
            log->debug("[local] file is not JPEG/PNG (" + std::to_string(art.bytes.size()) +
                       " bytes); ffmpeg embed may still accept it");
        }
    }
    art.mimeType = guessMime(art.bytes);
    art.source = "local:" + path;
    if (log) {
        log->info("[local] loaded cover (" + art.mimeType + ", " +
                  std::to_string(art.bytes.size()) + " bytes)");
    }
    return art;
}

}  // namespace optigrab
