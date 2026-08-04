#include "optigrab/adapters/cover/MusicBrainzCoverArtProvider.hpp"

#include "optigrab/domain/DiscId.hpp"
#include "optigrab/util/Process.hpp"

#include <filesystem>
#include <fstream>
#include <regex>

namespace optigrab {
namespace {

constexpr const char* kUserAgent = "optigrab/0.2.0 (https://github.com/ciroque/optigrab)";

struct CurlResult {
    int exitCode{0};
    std::string output;
    bool fileOk{false};
    std::uintmax_t fileSize{0};
};

CurlResult curlToFile(const std::string& curlBin, const std::string& url,
                      const std::filesystem::path& outFile) {
    const std::vector<std::string> args = {
        curlBin, "-sS", "-L", "--fail", "--max-time", "30", "-A", kUserAgent,
        "-o",    outFile.string(),      url,
    };
    CurlResult r;
    r.exitCode = runProcess(args, r.output, r.output);
    std::error_code ec;
    if (std::filesystem::exists(outFile, ec)) {
        r.fileSize = std::filesystem::file_size(outFile, ec);
        r.fileOk = !ec && r.fileSize > 0;
    }
    return r;
}

std::string readFile(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

std::string trimSnippet(std::string s, std::size_t max = 240) {
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

std::optional<std::string> firstReleaseId(const std::string& json) {
    const auto releasesPos = json.find("\"releases\"");
    const std::string scope =
        releasesPos == std::string::npos ? json : json.substr(releasesPos);
    static const std::regex re(
        "\"id\"\\s*:\\s*\"([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-"
        "[0-9a-fA-F]{4}-[0-9a-fA-F]{12})\"");
    std::smatch m;
    if (std::regex_search(scope, m, re) && m.size() >= 2) {
        return m[1].str();
    }
    return std::nullopt;
}

bool looksLikeErrorJson(const std::string& json) {
    return json.find("\"error\"") != std::string::npos ||
           json.find("Not Found") != std::string::npos;
}

// Extract "503" from messages like: curl: (22) The requested URL returned error: 503
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

void logCurlFailure(Logger* log, const std::string& step, const std::string& url,
                    const CurlResult& r) {
    if (!log) {
        return;
    }
    log->warn("[mb] " + step + " failed");
    log->debug("[mb]   url: " + url);
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
    log->warn("[mb]   " + why);
    if (!r.output.empty()) {
        log->debug("[mb]   curl: " + trimSnippet(r.output));
    }
    if (!r.fileOk) {
        log->debug("[mb]   no usable download file (size " + std::to_string(r.fileSize) + ")");
    }

    if (http) {
        if (*http == 404 || *http == 400) {
            log->info("[mb] hint: resource not found — disc/release may be unknown to the service");
        } else if (*http == 503 || *http == 502 || *http == 504) {
            log->info("[mb] hint: service temporarily unavailable — retry later (not a disc-ID problem)");
        } else if (*http == 429) {
            log->info("[mb] hint: rate limited by the service — wait and retry");
        } else if (*http >= 500) {
            log->info("[mb] hint: remote server error — retry later");
        }
    } else if (r.exitCode == 28) {
        log->info("[mb] hint: network timeout — check connectivity and retry");
    } else if (r.exitCode == 6 || r.exitCode == 7) {
        log->info("[mb] hint: network/DNS problem — check connectivity");
    } else if (r.exitCode == 127) {
        log->info("[mb] hint: install curl and ensure it is on PATH");
    }
}

}  // namespace

MusicBrainzCoverArtProvider::MusicBrainzCoverArtProvider(std::string curlBinary)
    : curl_(std::move(curlBinary)) {}

std::optional<CoverArt> MusicBrainzCoverArtProvider::fetch(const DiscInfo& disc, const Session&,
                                                           Logger* log) {
    int audioTracks = 0;
    for (const auto& t : disc.tracks) {
        if (t.audio) {
            ++audioTracks;
        }
    }
    if (log) {
        log->debug("[mb] disc has " + std::to_string(disc.tracks.size()) + " TOC entries, " +
                   std::to_string(audioTracks) + " audio");
    }

    const auto discId = computeMusicBrainzDiscId(disc);
    if (!discId) {
        if (log) {
            log->warn("[mb] cannot compute MusicBrainz Disc ID (need audio tracks with LBA/length)");
        }
        return std::nullopt;
    }
    if (log) {
        log->info("[mb] disc ID: " + *discId);
    }

    const auto tmp = std::filesystem::temp_directory_path() / ("optigrab-mb-" + *discId + ".json");
    const std::string mbUrl =
        "https://musicbrainz.org/ws/2/discid/" + *discId + "?fmt=json&inc=artists";
    if (log) {
        log->info("[mb] querying MusicBrainz ...");
        log->debug("[mb] GET " + mbUrl);
    }

    auto mb = curlToFile(curl_, mbUrl, tmp);
    if (mb.exitCode != 0 || !mb.fileOk) {
        logCurlFailure(log, "MusicBrainz discid lookup", mbUrl, mb);
        std::error_code ec;
        std::filesystem::remove(tmp, ec);
        return std::nullopt;
    }

    const auto json = readFile(tmp);
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    if (log) {
        log->debug("[mb] MusicBrainz response " + std::to_string(json.size()) + " bytes");
    }
    if (looksLikeErrorJson(json)) {
        if (log) {
            log->warn("[mb] response looks like an error: " + trimSnippet(json));
        }
        return std::nullopt;
    }
    if (json.find("\"releases\"") == std::string::npos) {
        if (log) {
            log->warn("[mb] JSON has no \"releases\" field — disc not linked to a release");
            log->debug("[mb] snippet: " + trimSnippet(json));
        }
        return std::nullopt;
    }

    const auto releaseId = firstReleaseId(json);
    if (!releaseId) {
        if (log) {
            log->warn("[mb] could not parse a release UUID from MusicBrainz JSON");
            log->debug("[mb] snippet: " + trimSnippet(json));
        }
        return std::nullopt;
    }
    if (log) {
        log->info("[mb] release MBID: " + *releaseId);
    }

    const auto imgPath =
        std::filesystem::temp_directory_path() / ("optigrab-cover-" + *releaseId + ".img");
    const std::string caa500 = "https://coverartarchive.org/release/" + *releaseId + "/front-500";
    if (log) {
        log->info("[mb] fetching Cover Art Archive front-500 ...");
        log->debug("[mb] GET " + caa500);
    }
    auto caa = curlToFile(curl_, caa500, imgPath);
    if (caa.exitCode != 0 || !caa.fileOk) {
        logCurlFailure(log, "CAA front-500", caa500, caa);
        if (log) {
            log->info("[mb] trying CAA full front ...");
        }
        const std::string caaFront =
            "https://coverartarchive.org/release/" + *releaseId + "/front";
        caa = curlToFile(curl_, caaFront, imgPath);
        if (caa.exitCode != 0 || !caa.fileOk) {
            logCurlFailure(log, "CAA front", caaFront, caa);
            const auto st = httpStatusFromCurlOutput(caa.output);
            if (st && *st == 404 && log) {
                log->info("[mb] hint: release exists but Cover Art Archive has no front image");
            }
            std::filesystem::remove(imgPath, ec);
            return std::nullopt;
        }
    }

    CoverArt art;
    {
        std::ifstream in(imgPath, std::ios::binary);
        art.bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
    std::filesystem::remove(imgPath, ec);

    if (art.bytes.empty()) {
        if (log) {
            log->warn("[mb] downloaded cover file was empty");
        }
        return std::nullopt;
    }
    if (!isJpeg(art.bytes) && !isPng(art.bytes)) {
        if (log) {
            log->warn("[mb] downloaded bytes are not JPEG/PNG (size " +
                      std::to_string(art.bytes.size()) + "); rejecting");
            std::string head;
            for (std::size_t i = 0; i < art.bytes.size() && i < 80; ++i) {
                const auto c = static_cast<char>(art.bytes[i]);
                head.push_back((c >= 32 && c < 127) ? c : '.');
            }
            log->debug("[mb] head: " + head);
        }
        return std::nullopt;
    }

    art.mimeType = guessMime(art.bytes);
    art.source = "coverartarchive:" + *releaseId;
    if (log) {
        log->info("[mb] cover OK: " + art.mimeType + ", " + std::to_string(art.bytes.size()) +
                  " bytes");
    }
    return art;
}

}  // namespace optigrab
