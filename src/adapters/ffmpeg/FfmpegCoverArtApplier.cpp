#include "optigrab/adapters/ffmpeg/FfmpegCoverArtApplier.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/util/Process.hpp"

#include <fstream>

namespace optigrab {

FfmpegCoverArtApplier::FfmpegCoverArtApplier(std::string binary) : binary_(std::move(binary)) {}

std::filesystem::path FfmpegCoverArtApplier::writeSidecar(const std::filesystem::path& albumDir,
                                                          const CoverArt& art, LogFn log) {
    std::filesystem::create_directories(albumDir);
    const auto path = albumDir / ("cover" + sidecarExtension(art));
    std::ofstream out(path, std::ios::binary);
    if (!out) {
        throw OptigrabError("Cannot write cover sidecar: " + path.string());
    }
    out.write(reinterpret_cast<const char*>(art.bytes.data()),
              static_cast<std::streamsize>(art.bytes.size()));
    if (log) {
        log("Wrote cover art " + path.string() + " (from " + art.source + ")");
    }
    return path;
}

void FfmpegCoverArtApplier::embed(const std::filesystem::path& mp3Path, const CoverArt& art,
                                  LogFn log) {
    const auto coverFile =
        std::filesystem::temp_directory_path() /
        ("optigrab-embed-cover" + sidecarExtension(art));
    {
        std::ofstream out(coverFile, std::ios::binary);
        out.write(reinterpret_cast<const char*>(art.bytes.data()),
                  static_cast<std::streamsize>(art.bytes.size()));
    }

    const auto tmpOut = mp3Path.string() + ".cover-tmp.mp3";
    // Map audio from original; attach cover as video stream with attached_pic disposition.
    const std::vector<std::string> args = {
        binary_,
        "-hide_banner",
        "-loglevel",
        "error",
        "-y",
        "-i",
        mp3Path.string(),
        "-i",
        coverFile.string(),
        "-map",
        "0:a",
        "-map",
        "1:0",
        "-c",
        "copy",
        "-c:v",
        "mjpeg",
        "-disposition:v",
        "attached_pic",
        "-id3v2_version",
        "3",
        tmpOut,
    };

    try {
        runProcessOrThrow(args, "ffmpeg cover embed");
        std::error_code ec;
        std::filesystem::rename(tmpOut, mp3Path, ec);
        if (ec) {
            std::filesystem::copy_file(tmpOut, mp3Path,
                                       std::filesystem::copy_options::overwrite_existing, ec);
            std::filesystem::remove(tmpOut, ec);
            if (ec) {
                throw OptigrabError("Failed to replace MP3 after cover embed: " + mp3Path.string());
            }
        }
        if (log) {
            log("Embedded cover art into " + mp3Path.filename().string());
        }
    } catch (...) {
        std::error_code ec;
        std::filesystem::remove(tmpOut, ec);
        std::filesystem::remove(coverFile, ec);
        throw;
    }
    std::error_code ec;
    std::filesystem::remove(coverFile, ec);
}

}  // namespace optigrab
