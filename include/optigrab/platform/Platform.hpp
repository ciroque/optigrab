#pragma once

#include "optigrab/domain/Types.hpp"

namespace optigrab {

// Compile-time platform helpers for defaults and help text.
inline constexpr bool isWindows() {
#ifdef _WIN32
    return true;
#else
    return false;
#endif
}

inline constexpr bool isUnix() {
#ifdef _WIN32
    return false;
#else
    return true;
#endif
}

// Preferred extract backend for new sessions on this platform.
inline constexpr ExtractorKind defaultExtractor() {
#ifdef _WIN32
    return ExtractorKind::Ffmpeg;
#elif defined(__APPLE__)
    // Homebrew has no classic "cdparanoia" formula; use linked libcdio_paranoia.
    return ExtractorKind::LibcdioParanoia;
#else
    return ExtractorKind::Cdparanoia;
#endif
}

inline constexpr const char* platformName() {
#ifdef _WIN32
    return "Windows";
#elif defined(__APPLE__)
    return "macOS";
#else
    return "Linux";
#endif
}

}  // namespace optigrab
