#include "optigrab/adapters/ffmpeg/FfmpegExtractor.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/util/Process.hpp"

namespace optigrab {

FfmpegExtractor::FfmpegExtractor(std::string binary) : binary_(std::move(binary)) {}

void FfmpegExtractor::extractTrack(const std::string& devicePath,
                                   const TrackInfo& track,
                                   const std::filesystem::path& outputWav,
                                   ProgressFn progress) {
    if (!track.audio) {
        throw ExtractError("Track is not audio");
    }
    if (progress) {
        progress("ffmpeg extract track " + std::to_string(track.number));
    }

    // Use libcdio demuxer when available: -f libcdio -i /dev/sr0
    // Map audio stream track-1 (0-based).
    const int streamIndex = track.number - 1;
    const std::vector<std::string> args = {
        binary_,
        "-hide_banner",
        "-loglevel",
        "error",
        "-y",
        "-f",
        "libcdio",
        "-i",
        devicePath,
        "-map",
        "0:a:" + std::to_string(streamIndex),
        "-c:a",
        "pcm_s16le",
        outputWav.string(),
    };

    try {
        runProcessOrThrow(args, "ffmpeg extract");
    } catch (const OptigrabError& ex) {
        throw ExtractError(std::string(ex.what()) +
                           " (is ffmpeg built with libcdio? try: set extractor cdparanoia)");
    }
}

}  // namespace optigrab
