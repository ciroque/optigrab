#include "optigrab/services/RipService.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/services/Filename.hpp"

#include <chrono>
#include <filesystem>
#include <fstream>
#include <unistd.h>

namespace optigrab {
namespace {

void ensureParent(const std::filesystem::path& path) {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

}  // namespace

RipService::RipService(std::shared_ptr<TocReader> toc,
                       std::shared_ptr<AudioExtractor> extractor,
                       std::shared_ptr<AudioEncoder> encoder,
                       std::shared_ptr<MetadataProvider> metadata)
    : toc_(std::move(toc)),
      extractor_(std::move(extractor)),
      encoder_(std::move(encoder)),
      metadata_(std::move(metadata)) {}

void RipService::loadDisc(Session& session, LogFn log) {
    const auto& drive = session.selectedDrive();
    if (log) {
        log("Reading TOC from " + drive.path + " ...");
    }
    auto disc = toc_->readToc(drive.path);
    if (metadata_) {
        metadata_->enrich(disc);
    }
    // Session overrides win for album/artist labeling.
    if (session.album()) {
        disc.album = *session.album();
    }
    if (session.artist()) {
        disc.albumArtist = *session.artist();
        for (auto& t : disc.tracks) {
            if (t.artist.empty()) {
                t.artist = *session.artist();
            }
        }
    }
    session.setDisc(std::move(disc));
    if (log) {
        log("Loaded " + std::to_string(session.disc().tracks.size()) + " track(s).");
    }
}

Tags RipService::makeTags(const Session& session, const TrackInfo& track, int trackTotal) const {
    Tags tags;
    tags.trackNumber = track.number;
    tags.trackTotal = trackTotal;
    tags.title = track.title.empty() ? ("Track " + std::to_string(track.number)) : track.title;
    tags.artist = track.artist;
    if (tags.artist.empty() && session.artist()) {
        tags.artist = *session.artist();
    }
    if (tags.artist.empty()) {
        tags.artist = "Unknown Artist";
    }

    if (session.album()) {
        tags.album = *session.album();
    } else if (session.hasDisc() && session.disc().album) {
        tags.album = *session.disc().album;
    } else {
        tags.album = "Unknown Album";
    }

    if (session.artist()) {
        tags.albumArtist = *session.artist();
    } else if (session.hasDisc() && session.disc().albumArtist) {
        tags.albumArtist = *session.disc().albumArtist;
    } else {
        tags.albumArtist = tags.artist;
    }
    return tags;
}

std::vector<RipResult> RipService::ripTracks(Session& session,
                                             const std::vector<int>& trackNumbers,
                                             LogFn log) {
    if (session.ripInProgress()) {
        throw SessionError("A rip is already in progress");
    }
    if (!session.hasSelectedDrive()) {
        throw SessionError("No drive selected. Use: select drive <index|path>");
    }

    if (!session.hasDisc()) {
        loadDisc(session, log);
    }

    const auto& disc = session.disc();
    const int total = static_cast<int>(disc.tracks.size());
    std::vector<RipResult> results;
    results.reserve(trackNumbers.size());

    session.setRipInProgress(true);
    try {
        const auto tmpDir =
            std::filesystem::temp_directory_path() / ("optigrab-" + std::to_string(::getpid()));
        std::filesystem::create_directories(tmpDir);

        for (int n : trackNumbers) {
            RipResult result;
            result.trackNumber = n;
            if (n < 1 || n > total) {
                result.success = false;
                result.message = "Track out of range";
                results.push_back(std::move(result));
                continue;
            }

            const auto& track = disc.tracks[static_cast<std::size_t>(n - 1)];
            if (!track.audio) {
                result.success = false;
                result.message = "Not an audio track";
                results.push_back(std::move(result));
                continue;
            }

            try {
                const auto tags = makeTags(session, track, total);
                const auto outMp3 = buildTrackPath(session.outputDirectory(), tags);
                ensureParent(outMp3);

                const auto wav = tmpDir / ("track-" + std::to_string(n) + ".wav");
                if (log) {
                    log("Extracting track " + std::to_string(n) + " via " + extractor_->name() +
                        " ...");
                }
                extractor_->extractTrack(session.selectedDrive().path, track, wav, log);

                if (log) {
                    log("Encoding track " + std::to_string(n) + " via " + encoder_->name() + " ...");
                }
                encoder_->encode(wav, outMp3, tags, session.quality(), log);

                std::error_code ec;
                std::filesystem::remove(wav, ec);

                result.success = true;
                result.outputPath = outMp3;
                result.message = "OK";
                if (log) {
                    log("Wrote " + outMp3.string());
                }
            } catch (const std::exception& ex) {
                result.success = false;
                result.message = ex.what();
                if (log) {
                    log("Track " + std::to_string(n) + " failed: " + ex.what());
                }
            }
            results.push_back(std::move(result));
        }

        std::error_code ec;
        std::filesystem::remove_all(tmpDir, ec);
    } catch (...) {
        session.setRipInProgress(false);
        throw;
    }
    session.setRipInProgress(false);
    return results;
}

}  // namespace optigrab
