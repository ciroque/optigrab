#pragma once

#include <cstdint>
#include <optional>
#include <string>
#include <vector>

namespace optigrab {

struct DriveInfo {
    std::string path;   // e.g. /dev/sr0
    std::string model;  // human-readable model, may be empty
    int index{0};       // enumeration index
};

struct TrackInfo {
    int number{0};          // 1-based track number
    std::int64_t startLba{0};
    std::int64_t sectors{0}; // CD-DA sectors (2352 bytes each when raw)
    bool audio{true};
    std::string title;       // may be empty until metadata resolved
    std::string artist;      // may be empty
};

struct DiscInfo {
    std::string devicePath;
    std::vector<TrackInfo> tracks;
    std::optional<std::string> album;
    std::optional<std::string> albumArtist;
};

struct Tags {
    std::string title;
    std::string artist;
    std::string album;
    std::string albumArtist;
    int trackNumber{0};
    int trackTotal{0};
    std::optional<int> year;
};

struct AudioFormat {
    int sampleRate{44100};
    int channels{2};
    int bitsPerSample{16};
};

enum class ExtractorKind {
    Ffmpeg,
    Cdparanoia,
    LibcdioParanoia,
};

enum class EncoderKind {
    Ffmpeg,
};

enum class QualityPreset {
    V0,   // lame VBR -V0 via ffmpeg
    V2,
    Cbr192,
    Cbr256,
    Cbr320,
};

// When cover lookup finds nothing (and cover fetch is enabled).
enum class CoverMissingPolicy {
    Ask,       // prompt if TTY; otherwise abort
    Continue,  // warn and rip without cover
    Abort,     // stop before extract
};

inline const char* toString(ExtractorKind k) {
    switch (k) {
    case ExtractorKind::Ffmpeg: return "ffmpeg";
    case ExtractorKind::Cdparanoia: return "cdparanoia";
    case ExtractorKind::LibcdioParanoia: return "libcdio";
    }
    return "unknown";
}

inline const char* toString(EncoderKind k) {
    switch (k) {
    case EncoderKind::Ffmpeg: return "ffmpeg";
    }
    return "unknown";
}

inline const char* toString(QualityPreset q) {
    switch (q) {
    case QualityPreset::V0: return "V0";
    case QualityPreset::V2: return "V2";
    case QualityPreset::Cbr192: return "192";
    case QualityPreset::Cbr256: return "256";
    case QualityPreset::Cbr320: return "320";
    }
    return "unknown";
}

inline const char* toString(CoverMissingPolicy p) {
    switch (p) {
    case CoverMissingPolicy::Ask: return "ask";
    case CoverMissingPolicy::Continue: return "continue";
    case CoverMissingPolicy::Abort: return "abort";
    }
    return "unknown";
}

}  // namespace optigrab
