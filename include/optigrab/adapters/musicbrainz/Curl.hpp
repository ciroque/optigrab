#pragma once

#include "optigrab/log/Logger.hpp"

#include <cstdint>
#include <filesystem>
#include <optional>
#include <string>

namespace optigrab {

// MusicBrainz asks every client to identify itself.
inline constexpr const char* kMusicBrainzUserAgent =
    "optigrab/0.2.0 (https://github.com/ciroque/optigrab)";

struct CurlResult {
    int exitCode{0};
    std::string output;
    bool fileOk{false};
    std::uintmax_t fileSize{0};
};

// Download url to outFile via the curl binary (follows redirects, retries).
// strictHttpFail: pass --fail so 4xx/5xx yield a nonzero exit instead of an error body.
CurlResult curlToFile(const std::string& curlBin, const std::string& url,
                      const std::filesystem::path& outFile, bool strictHttpFail = false);

// HTTP status parsed from curl's "--fail" message, if any.
[[nodiscard]] std::optional<int> httpStatusFromCurlOutput(const std::string& output);

// One-line human reason for a failed download, e.g. "curl exit 22 (HTTP 503)".
[[nodiscard]] std::string describeCurlFailure(const CurlResult& r);

// Warn with the reason plus debug/info hints ([mb]-prefixed).
void logCurlFailure(Logger* log, const std::string& step, const std::string& url,
                    const CurlResult& r);

[[nodiscard]] std::string trimSnippet(std::string s, std::size_t max = 240);

[[nodiscard]] std::string readWholeFile(const std::filesystem::path& p);

}  // namespace optigrab
