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

    // Prefer libcdio demuxer (Linux builds of ffmpeg). On Windows, try the same
    // and fall back to a digital-copy friendly device path if needed.
    const int streamIndex = track.number - 1;
    std::string input = devicePath;
#ifdef _WIN32
    // ffmpeg on Windows often wants the drive root, e.g. D:
    if (input.size() >= 2 && input[1] == ':' && input.back() != ':' && input.find('\\') == std::string::npos) {
        // keep "D:" as-is
    }
#endif

    const std::vector<std::string> args = {
        binary_,
        "-hide_banner",
        "-loglevel",
        "error",
        "-y",
        "-f",
        "libcdio",
        "-i",
        input,
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
