#include "optigrab/adapters/musicbrainz/Curl.hpp"

#include "optigrab/util/Process.hpp"

#include <fstream>
#include <regex>
#include <vector>

namespace optigrab {

#ifndef OPTIGRAB_VERSION_STRING
#define OPTIGRAB_VERSION_STRING "dev"
#endif

std::string musicBrainzUserAgent(std::string_view version) {
    if (!version.empty() && (version.front() == 'v' || version.front() == 'V')) {
        version.remove_prefix(1);
    }
    if (version.empty()) {
        version = "dev";
    }
    return "optigrab/" + std::string(version) + " (https://github.com/ciroque/optigrab)";
}

std::string musicBrainzUserAgent() { return musicBrainzUserAgent(OPTIGRAB_VERSION_STRING); }

std::string readWholeFile(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// Cover Art Archive returns 307 → archive.org. That hop is flaky (intermittent 500).
// Do NOT use curl --fail for CAA: it aborts on those 5xxs even when a retry would succeed.
// Follow redirects, retry a few times, then validate bytes are a real image.
CurlResult curlToFile(const std::string& curlBin, const std::string& url,
                      const std::filesystem::path& outFile, bool strictHttpFail) {
    std::vector<std::string> args = {
        curlBin,
        "-sS",
        "-L",
        "--max-time",
        "45",
        "--retry",
        "3",
        "--retry-delay",
        "1",
        "--retry-all-errors",
        "-A",
        musicBrainzUserAgent(),
        "-o",
        outFile.string(),
        url,
    };
    if (strictHttpFail) {
        // MusicBrainz JSON: hard-fail on 4xx so we don't parse HTML error pages.
        args.insert(args.begin() + 3, "--fail");
    }
    CurlResult r;
    r.exitCode = runProcess(args, r.output, r.output);
    std::error_code ec;
    if (std::filesystem::exists(outFile, ec)) {
        r.fileSize = std::filesystem::file_size(outFile, ec);
        r.fileOk = !ec && r.fileSize > 0;
        // Redirect body without following is plain text: "See: https://..."
        if (r.fileOk && r.fileSize < 512) {
            const auto head = readWholeFile(outFile);
            if (head.rfind("See: http", 0) == 0 ||
                head.find("https://archive.org/") != std::string::npos) {
                static const std::regex urlRe(R"(https://[^\s]+)");
                std::smatch m;
                if (std::regex_search(head, m, urlRe)) {
                    std::string cleaned = m[0].str();
                    while (!cleaned.empty() && (cleaned.back() == '.' || cleaned.back() == ')' ||
                                                cleaned.back() == '\r')) {
                        cleaned.pop_back();
                    }
                    std::vector<std::string> args2 = {
                        curlBin,         "-sS", "-L", "--max-time", "45", "--retry", "3",
                        "--retry-delay", "1",   "--retry-all-errors", "-A", musicBrainzUserAgent(),
                        "-o",            outFile.string(), cleaned,
                    };
                    r.exitCode = runProcess(args2, r.output, r.output);
                    if (std::filesystem::exists(outFile, ec)) {
                        r.fileSize = std::filesystem::file_size(outFile, ec);
                        r.fileOk = !ec && r.fileSize > 64;
                    } else {
                        r.fileOk = false;
                    }
                }
            }
        }
        // Final gate for non-strict downloads: junk HTML counts as failure later via isJpeg/isPng.
        if (r.fileOk && r.exitCode != 0 && !strictHttpFail) {
            // Prefer success if we have a substantial payload (image), even if curl reported error.
            r.exitCode = 0;
        }
    }
    return r;
}

std::string trimSnippet(std::string s, std::size_t max) {
    while (!s.empty() && (s.back() == '\n' || s.back() == '\r' || s.back() == ' ')) {
        s.pop_back();
    }
    for (char& c : s) {
        if (c == '\n' || c == '\r') {
            c = ' ';
        }
    }
    if (s.size() > max) {
        s.resize(max);
        s += "...";
    }
    return s;
}

std::optional<int> httpStatusFromCurlOutput(const std::string& output) {
    static const std::regex re(R"(returned error:\s*(\d{3}))");
    std::smatch m;
    if (std::regex_search(output, m, re) && m.size() >= 2) {
        try {
            return std::stoi(m[1].str());
        } catch (...) {
            return std::nullopt;
        }
    }
    return std::nullopt;
}

std::string describeCurlFailure(const CurlResult& r) {
    std::string why = "curl exit " + std::to_string(r.exitCode);
    const auto http = httpStatusFromCurlOutput(r.output);
    if (http) {
        why += " (HTTP " + std::to_string(*http) + ")";
    } else if (r.exitCode == 22) {
        why += " (HTTP error)";
    } else if (r.exitCode == 6) {
        why += " (couldn't resolve host)";
    } else if (r.exitCode == 7) {
        why += " (failed to connect)";
    } else if (r.exitCode == 28) {
        why += " (timeout)";
    } else if (r.exitCode == 127) {
        why += " (curl not found on PATH?)";
    }
    return why;
}

void logCurlFailure(Logger* log, const std::string& step, const std::string& url,
                    const CurlResult& r) {
    if (!log) {
        return;
    }
    log->warn("[mb] " + step + " failed");
    log->debug("[mb]   url: " + url);
    log->warn("[mb]   " + describeCurlFailure(r));
    if (!r.output.empty()) {
        log->debug("[mb]   curl: " + trimSnippet(r.output));
    }

    const auto http = httpStatusFromCurlOutput(r.output);
    if (http) {
        if (*http == 404 || *http == 400) {
            log->info("[mb] hint: no cover (or resource) at this URL");
        } else if (*http == 503 || *http == 502 || *http == 504) {
            log->info("[mb] hint: service temporarily unavailable — retry later");
        } else if (*http == 429) {
            log->info("[mb] hint: rate limited — wait and retry");
        } else if (*http >= 500) {
            log->info("[mb] hint: remote server error — retry later");
        }
    }
}

}  // namespace optigrab
