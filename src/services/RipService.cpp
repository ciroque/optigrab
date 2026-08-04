#include "optigrab/services/RipService.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/services/Filename.hpp"

#include <filesystem>
#include <fstream>

#ifdef _WIN32
#include <process.h>
#else
#include <unistd.h>
#endif

namespace optigrab {
namespace {

void ensureParent(const std::filesystem::path& path) {
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent);
    }
}

std::string trackLabel(const TrackInfo& track) {
    if (!track.title.empty()) {
        return track.title;
    }
    return "Track " + std::to_string(track.number);
}

}  // namespace

RipService::RipService(std::shared_ptr<TocReader> toc,
                       std::shared_ptr<AudioExtractor> extractor,
                       std::shared_ptr<AudioEncoder> encoder,
                       std::shared_ptr<MetadataProvider> metadata,
                       std::shared_ptr<CoverArtProvider> coverProvider,
                       std::shared_ptr<CoverArtApplier> coverApplier)
    : toc_(std::move(toc)),
      extractor_(std::move(extractor)),
      encoder_(std::move(encoder)),
      metadata_(std::move(metadata)),
      coverProvider_(std::move(coverProvider)),
      coverApplier_(std::move(coverApplier)) {}

void RipService::loadDisc(Session& session, LogFn log) {
    const auto& drive = session.selectedDrive();
    if (log) {
        log("Reading TOC from " + drive.path + " ...");
    }
    auto disc = toc_->readToc(drive.path);
    if (metadata_) {
        metadata_->enrich(disc);
    }
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
    const int totalOnDisc = static_cast<int>(disc.tracks.size());
    const int jobCount = static_cast<int>(trackNumbers.size());
    std::vector<RipResult> results;
    results.reserve(trackNumbers.size());

    // --- Pass 0: cover download (fail soft) ---
    std::optional<CoverArt> cover;
    if (session.fetchCoverArt() && coverProvider_) {
        if (log) {
            log("Looking up cover art via " + coverProvider_->name() + " ...");
        }
        try {
            cover = coverProvider_->fetch(disc, session);
            if (cover && log) {
                log("Cover art ready (" + cover->source + ", " +
                    std::to_string(cover->bytes.size()) + " bytes).");
            } else if (log) {
                log("No cover art found (continuing without).");
            }
        } catch (const std::exception& ex) {
            if (log) {
                log(std::string("Cover art lookup failed: ") + ex.what() + " (continuing).");
            }
            cover.reset();
        }
    }

    session.setRipInProgress(true);
    try {
#ifdef _WIN32
        const auto pid = static_cast<long>(::_getpid());
#else
        const auto pid = static_cast<long>(::getpid());
#endif
        const auto tmpDir =
            std::filesystem::temp_directory_path() / ("optigrab-" + std::to_string(pid));
        std::filesystem::create_directories(tmpDir);

        std::vector<std::filesystem::path> successfulMp3s;
        std::filesystem::path albumDir;

        // --- Pass A: extract + encode ---
        int jobIndex = 0;
        for (int n : trackNumbers) {
            ++jobIndex;
            RipResult result;
            result.trackNumber = n;
            if (n < 1 || n > totalOnDisc) {
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

            const std::string prefix =
                "[" + std::to_string(jobIndex) + "/" + std::to_string(jobCount) + "] ";

            try {
                const auto tags = makeTags(session, track, totalOnDisc);
                const auto outMp3 = buildTrackPath(session.outputDirectory(), tags);
                ensureParent(outMp3);
                if (albumDir.empty()) {
                    albumDir = outMp3.parent_path();
                }

                const auto wav = tmpDir / ("track-" + std::to_string(n) + ".wav");
                if (log) {
                    log(prefix + "Extracting \"" + trackLabel(track) + "\" via " +
                        extractor_->name() + " ...");
                }
                extractor_->extractTrack(session.selectedDrive().path, track, wav,
                                         [&](const std::string& m) {
                                             if (log) {
                                                 log(m);
                                             }
                                         });

                if (log) {
                    log(prefix + "Encoding via " + encoder_->name() + " (" +
                        toString(session.quality()) + ") ...");
                }
                encoder_->encode(wav, outMp3, tags, session.quality(),
                                 [&](const std::string& m) {
                                     if (log) {
                                         log(m);
                                     }
                                 });

                std::error_code ec;
                std::filesystem::remove(wav, ec);

                result.success = true;
                result.outputPath = outMp3;
                result.message = "OK";
                successfulMp3s.push_back(outMp3);
                if (log) {
                    log(prefix + "Wrote " + outMp3.string());
                }
            } catch (const std::exception& ex) {
                result.success = false;
                result.message = ex.what();
                if (log) {
                    log(prefix + "FAILED: " + ex.what());
                }
            }
            results.push_back(std::move(result));
        }

        // --- Pass B: sidecar + embed (serial, only if cover exists) ---
        if (cover && coverApplier_ && !successfulMp3s.empty()) {
            if (albumDir.empty()) {
                albumDir = successfulMp3s.front().parent_path();
            }
            try {
                if (log) {
                    log("Applying cover art (" + coverApplier_->name() + ") ...");
                }
                coverApplier_->writeSidecar(albumDir, *cover, log);
                int i = 0;
                const int n = static_cast<int>(successfulMp3s.size());
                for (const auto& mp3 : successfulMp3s) {
                    ++i;
                    try {
                        if (log) {
                            log("[" + std::to_string(i) + "/" + std::to_string(n) +
                                "] Embedding cover into " + mp3.filename().string() + " ...");
                        }
                        coverApplier_->embed(mp3, *cover, log);
                    } catch (const std::exception& ex) {
                        if (log) {
                            log("  cover embed failed for " + mp3.filename().string() + ": " +
                                ex.what());
                        }
                    }
                }
            } catch (const std::exception& ex) {
                if (log) {
                    log(std::string("Cover art apply failed: ") + ex.what() +
                        " (MP3s kept without art).");
                }
            }
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
