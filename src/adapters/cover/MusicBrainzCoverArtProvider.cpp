#include "optigrab/adapters/cover/MusicBrainzCoverArtProvider.hpp"

#include "optigrab/adapters/musicbrainz/Curl.hpp"
#include "optigrab/domain/DiscId.hpp"
#include "optigrab/domain/Errors.hpp"

#include <algorithm>
#include <filesystem>
#include <fstream>

namespace optigrab {
namespace {

std::optional<CoverArt> loadCoverFile(const std::filesystem::path& imgPath,
                                      const std::string& source, Logger* log) {
    CoverArt art;
    {
        std::ifstream in(imgPath, std::ios::binary);
        art.bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    if (art.bytes.empty()) {
        if (log) {
            log->warn("[mb] downloaded cover file was empty");
        }
        return std::nullopt;
    }
    if (!isJpeg(art.bytes) && !isPng(art.bytes)) {
        if (log) {
            log->warn("[mb] downloaded bytes are not JPEG/PNG (size " +
                      std::to_string(art.bytes.size()) + "); rejecting");
        }
        return std::nullopt;
    }
    art.mimeType = guessMime(art.bytes);
    art.source = source;
    if (log) {
        log->info("[mb] cover OK: " + art.mimeType + ", " + std::to_string(art.bytes.size()) +
                  " bytes");
    }
    return art;
}

std::optional<CoverArt> fetchCaaFront(const std::string& curlBin, const std::string& releaseId,
                                      Logger* log) {
    const auto imgPath =
        std::filesystem::temp_directory_path() / ("optigrab-cover-" + releaseId + ".img");
    std::error_code ec;

    const std::string caa500 = "https://coverartarchive.org/release/" + releaseId + "/front-500";
    if (log) {
        log->info("[mb] fetching CAA front-500 for release " + releaseId);
        log->debug("[mb] GET " + caa500);
    }
    auto caa = curlToFile(curlBin, caa500, imgPath, /*strictHttpFail=*/false);
    if (caa.fileOk) {
        if (auto art = loadCoverFile(imgPath, "coverartarchive:" + releaseId, log)) {
            std::filesystem::remove(imgPath, ec);
            return art;
        }
    } else {
        logCurlFailure(log, "CAA front-500", caa500, caa);
    }

    const std::string caaFront = "https://coverartarchive.org/release/" + releaseId + "/front";
    if (log) {
        log->info("[mb] trying CAA full front for release " + releaseId);
        log->debug("[mb] GET " + caaFront);
    }
    caa = curlToFile(curlBin, caaFront, imgPath, /*strictHttpFail=*/false);
    if (caa.fileOk) {
        if (auto art = loadCoverFile(imgPath, "coverartarchive:" + releaseId, log)) {
            std::filesystem::remove(imgPath, ec);
            return art;
        }
    } else {
        logCurlFailure(log, "CAA front", caaFront, caa);
    }
    std::filesystem::remove(imgPath, ec);
    return std::nullopt;
}

}  // namespace

MusicBrainzCoverArtProvider::MusicBrainzCoverArtProvider(std::shared_ptr<ReleaseLookup> releases,
                                                         std::string curlBinary)
    : releases_(std::move(releases)), curl_(std::move(curlBinary)) {}

std::optional<CoverArt> MusicBrainzCoverArtProvider::fetch(const DiscInfo& disc,
                                                           const Session& session, Logger* log) {
    int audioTracks = 0;
    for (const auto& t : disc.tracks) {
        if (t.audio) {
            ++audioTracks;
        }
    }
    if (log) {
        log->debug("[mb] disc has " + std::to_string(disc.tracks.size()) + " TOC entries, " +
                   std::to_string(audioTracks) + " audio");
    }

    const auto discId = computeMusicBrainzDiscId(disc);
    if (!discId) {
        if (log) {
            log->warn("[mb] cannot compute MusicBrainz Disc ID (need audio tracks with LBA/length)");
        }
        return std::nullopt;
    }
    if (log) {
        log->info("[mb] disc ID: " + *discId);
    }

    // Reuse the session's lookup for this disc rather than querying MusicBrainz again.
    std::vector<Release> candidates;
    const auto& cached = session.discLookup();
    if (cached && cached->discId == *discId) {
        if (log) {
            log->debug("[mb] using " + std::to_string(cached->releases.size()) +
                       " release(s) from session lookup");
        }
        candidates = cached->releases;
    } else {
        try {
            candidates = releases_->lookup(*discId, log).releases;
        } catch (const LookupError& ex) {
            if (log) {
                log->warn(std::string("[mb] ") + ex.what());
            }
            return std::nullopt;
        }
    }

    if (candidates.empty()) {
        if (log) {
            log->warn("[mb] no releases linked to this disc ID");
        }
        return std::nullopt;
    }
    // Releases advertising a front image first; keep MusicBrainz order otherwise.
    std::stable_sort(candidates.begin(), candidates.end(),
                     [](const Release& a, const Release& b) {
                         return a.hasFrontCover > b.hasFrontCover;
                     });
    // The user's chosen release (select release) goes before everything else.
    if (const auto* chosen = session.selectedRelease();
        chosen && cached && cached->discId == *discId) {
        std::stable_partition(candidates.begin(), candidates.end(),
                              [&](const Release& r) { return r.id == chosen->id; });
    }

    for (const auto& c : candidates) {
        if (log) {
            log->debug("[mb]   release " + c.id +
                       (c.hasFrontCover ? " (front cover advertised)" : " (no front flag)"));
        }
    }

    for (const auto& c : candidates) {
        if (log) {
            log->info("[mb] trying release MBID: " + c.id);
        }
        if (auto art = fetchCaaFront(curl_, c.id, log)) {
            return art;
        }
    }

    if (log) {
        log->warn("[mb] no Cover Art Archive front image for any linked release");
    }
    return std::nullopt;
}

}  // namespace optigrab
