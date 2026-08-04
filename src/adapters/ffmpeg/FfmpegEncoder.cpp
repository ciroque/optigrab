#include "optigrab/adapters/ffmpeg/FfmpegEncoder.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/util/Process.hpp"

namespace optigrab {
namespace {

void appendQuality(std::vector<std::string>& args, QualityPreset quality) {
    args.push_back("-c:a");
    args.push_back("libmp3lame");
    switch (quality) {
    case QualityPreset::V0:
        args.push_back("-q:a");
        args.push_back("0");
        break;
    case QualityPreset::V2:
        args.push_back("-q:a");
        args.push_back("2");
        break;
    case QualityPreset::Cbr192:
        args.push_back("-b:a");
        args.push_back("192k");
        break;
    case QualityPreset::Cbr256:
        args.push_back("-b:a");
        args.push_back("256k");
        break;
    case QualityPreset::Cbr320:
        args.push_back("-b:a");
        args.push_back("320k");
        break;
    }
}

}  // namespace

FfmpegEncoder::FfmpegEncoder(std::string binary) : binary_(std::move(binary)) {}

void FfmpegEncoder::encode(const std::filesystem::path& inputWav,
                           const std::filesystem::path& outputMp3,
                           const Tags& tags,
                           QualityPreset quality,
                           ProgressFn progress) {
    if (progress) {
        progress("ffmpeg encode " + outputMp3.string());
    }

    std::vector<std::string> args = {
        binary_, "-hide_banner", "-loglevel", "error", "-y", "-i", inputWav.string(),
    };
    appendQuality(args, quality);

    auto meta = [&](const char* key, const std::string& value) {
        if (value.empty()) {
            return;
        }
        args.push_back("-metadata");
        args.push_back(std::string(key) + "=" + value);
    };

    meta("title", tags.title);
    meta("artist", tags.artist);
    meta("album", tags.album);
    meta("album_artist", tags.albumArtist);
    if (tags.trackNumber > 0) {
        std::string tn = std::to_string(tags.trackNumber);
        if (tags.trackTotal > 0) {
            tn += "/" + std::to_string(tags.trackTotal);
        }
        meta("track", tn);
    }
    if (tags.year) {
        meta("date", std::to_string(*tags.year));
    }

    args.push_back(outputMp3.string());

    try {
        runProcessOrThrow(args, "ffmpeg encode");
    } catch (const OptigrabError& ex) {
        throw EncodeError(ex.what());
    }
}

}  // namespace optigrab
