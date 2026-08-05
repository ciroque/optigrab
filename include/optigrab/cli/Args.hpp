#pragma once

#include "optigrab/domain/Types.hpp"
#include "optigrab/log/LogLevel.hpp"

#include <optional>
#include <string>
#include <vector>

namespace optigrab {

struct LaunchArgs {
    bool showHelp{false};
    bool showVersion{false};
    bool interactive{true};

    std::optional<std::string> drive;  // index or path
    std::optional<std::string> outDir;
    std::optional<std::string> artist;
    std::optional<std::string> album;
    std::optional<std::string> coverPath;
    std::optional<bool> fetchCoverArt;
    // Default for one-shot: ask (applied when not interactive if unset still Ask via session).
    std::optional<CoverMissingPolicy> coverMissing;
    std::optional<FolderLayout> folderLayout;
    std::optional<LogLevel> logLevel;
    std::optional<QualityPreset> quality;
    std::optional<ExtractorKind> extractor;
    std::optional<EncoderKind> encoder;

    // Remaining tokens form one VERB NOUN command (e.g. rip track all).
    std::vector<std::string> command;
};

// Parse argv (excluding argv[0]). Throws ParseError on bad flags.
[[nodiscard]] LaunchArgs parseLaunchArgs(const std::vector<std::string>& argv);

[[nodiscard]] std::string usageText();

}  // namespace optigrab
