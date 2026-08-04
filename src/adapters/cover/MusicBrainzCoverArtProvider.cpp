#include "optigrab/adapters/cover/MusicBrainzCoverArtProvider.hpp"

#include "optigrab/domain/DiscId.hpp"
#include "optigrab/util/Process.hpp"

#include <algorithm>
#include <cctype>
#include <filesystem>
#include <fstream>
#include <regex>
#include <vector>

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

bool looksLikeErrorJson(const std::string& json) {
    return json.find("\"error\"") != std::string::npos ||
           json.find("Not Found") != std::string::npos;
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

bool isUuid(const std::string& s) {
    static const std::regex re(
        "^[0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12}$");
    return std::regex_match(s, re);
}

// Slice one top-level JSON object starting at '{' (string-aware brace match).
std::optional<std::string> sliceObject(const std::string& s, std::size_t& i) {
    while (i < s.size() && std::isspace(static_cast<unsigned char>(s[i]))) {
        ++i;
    }
    if (i >= s.size() || s[i] != '{') {
        return std::nullopt;
    }
    const std::size_t start = i;
    int depth = 0;
    bool inString = false;
    bool escape = false;
    for (; i < s.size(); ++i) {
        const char c = s[i];
        if (inString) {
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }
        if (c == '"') {
            inString = true;
            continue;
        }
        if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
            if (depth == 0) {
                ++i;
                return s.substr(start, i - start);
            }
        }
    }
    return std::nullopt;
}

// Find top-level JSON string value for key in object text "{...}".
std::optional<std::string> topLevelStringField(const std::string& obj, const std::string& key) {
    // Match "key" : "value" only when brace depth is 1 (inside this object, not nested).
    const std::string needle = "\"" + key + "\"";
    int depth = 0;
    bool inString = false;
    bool escape = false;
    for (std::size_t i = 0; i < obj.size(); ++i) {
        const char c = obj[i];
        if (inString) {
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }
        if (c == '"') {
            if (depth == 1 && obj.compare(i, needle.size(), needle) == 0) {
                std::size_t j = i + needle.size();
                while (j < obj.size() && std::isspace(static_cast<unsigned char>(obj[j]))) {
                    ++j;
                }
                if (j >= obj.size() || obj[j] != ':') {
                    inString = true;  // treat as normal string start
                    continue;
                }
                ++j;
                while (j < obj.size() && std::isspace(static_cast<unsigned char>(obj[j]))) {
                    ++j;
                }
                if (j >= obj.size() || obj[j] != '"') {
                    continue;
                }
                ++j;
                std::string val;
                while (j < obj.size()) {
                    const char vc = obj[j++];
                    if (vc == '\\') {
                        if (j < obj.size()) {
                            val.push_back(obj[j++]);
                        }
                        continue;
                    }
                    if (vc == '"') {
                        return val;
                    }
                    val.push_back(vc);
                }
                return std::nullopt;
            }
            inString = true;
            continue;
        }
        if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
        }
    }
    return std::nullopt;
}

bool topLevelCaaFrontTrue(const std::string& obj) {
    // Locate top-level "cover-art-archive" object, then look for "front": true inside it.
    const std::string key = "\"cover-art-archive\"";
    int depth = 0;
    bool inString = false;
    bool escape = false;
    for (std::size_t i = 0; i < obj.size(); ++i) {
        const char c = obj[i];
        if (inString) {
            if (escape) {
                escape = false;
            } else if (c == '\\') {
                escape = true;
            } else if (c == '"') {
                inString = false;
            }
            continue;
        }
        if (c == '"') {
            if (depth == 1 && obj.compare(i, key.size(), key) == 0) {
                std::size_t j = i + key.size();
                while (j < obj.size() && std::isspace(static_cast<unsigned char>(obj[j]))) {
                    ++j;
                }
                if (j >= obj.size() || obj[j] != ':') {
                    inString = true;
                    continue;
                }
                ++j;
                while (j < obj.size() && std::isspace(static_cast<unsigned char>(obj[j]))) {
                    ++j;
                }
                if (j >= obj.size() || obj[j] != '{') {
                    continue;
                }
                auto caa = sliceObject(obj, j);
                if (!caa) {
                    return false;
                }
                static const std::regex frontRe(R"("front"\s*:\s*true)");
                return std::regex_search(*caa, frontRe);
            }
            inString = true;
            continue;
        }
        if (c == '{') {
            ++depth;
        } else if (c == '}') {
            --depth;
        }
    }
    return false;
}

struct ReleaseCandidate {
    std::string id;
    bool hasFront{false};
};

std::vector<ReleaseCandidate> extractReleaseCandidates(const std::string& json) {
    std::vector<ReleaseCandidate> out;
    const auto pos = json.find("\"releases\"");
    if (pos == std::string::npos) {
        return out;
    }
    std::size_t i = json.find('[', pos);
    if (i == std::string::npos) {
        return out;
    }
    ++i;

    while (i < json.size()) {
        while (i < json.size() &&
               (std::isspace(static_cast<unsigned char>(json[i])) || json[i] == ',')) {
            ++i;
        }
        if (i >= json.size() || json[i] == ']') {
            break;
        }
        auto obj = sliceObject(json, i);
        if (!obj) {
            break;
        }
        auto id = topLevelStringField(*obj, "id");
        if (!id || !isUuid(*id)) {
            continue;
        }
        ReleaseCandidate c;
        c.id = *id;
        c.hasFront = topLevelCaaFrontTrue(*obj);
        out.push_back(std::move(c));
    }

    std::stable_sort(out.begin(), out.end(),
                     [](const ReleaseCandidate& a, const ReleaseCandidate& b) {
                         return a.hasFront > b.hasFront;
                     });
    return out;
}

std::optional<CoverArt> loadCoverFile(const std::filesystem::path& imgPath,
                                      const std::string& source, Logger* log) {
    CoverArt art;
    {
        std::ifstream in(imgPath, std::ios::binary);
        art.bytes.assign(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
    }
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
        }
        return std::nullopt;
    }
    art.mimeType = guessMime(art.bytes);
    art.source = source;
    if (log) {
        log->info("[mb] cover OK: " + art.mimeType + ", " + std::to_string(art.bytes.size()) +
                  " bytes");
    }
    return art;
}

std::optional<CoverArt> fetchCaaFront(const std::string& curlBin, const std::string& releaseId,
                                      Logger* log) {
    const auto imgPath =
        std::filesystem::temp_directory_path() / ("optigrab-cover-" + releaseId + ".img");
    std::error_code ec;

    const std::string caa500 = "https://coverartarchive.org/release/" + releaseId + "/front-500";
    if (log) {
        log->info("[mb] fetching CAA front-500 for release " + releaseId);
        log->debug("[mb] GET " + caa500);
    }
    auto caa = curlToFile(curlBin, caa500, imgPath);
    if (caa.exitCode == 0 && caa.fileOk) {
        auto art = loadCoverFile(imgPath, "coverartarchive:" + releaseId, log);
        std::filesystem::remove(imgPath, ec);
        return art;
    }
    logCurlFailure(log, "CAA front-500", caa500, caa);

    const std::string caaFront = "https://coverartarchive.org/release/" + releaseId + "/front";
    if (log) {
        log->info("[mb] trying CAA full front for release " + releaseId);
        log->debug("[mb] GET " + caaFront);
    }
    caa = curlToFile(curlBin, caaFront, imgPath);
    if (caa.exitCode == 0 && caa.fileOk) {
        auto art = loadCoverFile(imgPath, "coverartarchive:" + releaseId, log);
        std::filesystem::remove(imgPath, ec);
        return art;
    }
    logCurlFailure(log, "CAA front", caaFront, caa);
    std::filesystem::remove(imgPath, ec);
    return std::nullopt;
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

    auto candidates = extractReleaseCandidates(json);
    if (candidates.empty()) {
        if (log) {
            log->warn("[mb] could not parse any release MBID from MusicBrainz JSON");
            log->debug("[mb] snippet: " + trimSnippet(json));
        }
        return std::nullopt;
    }

    if (log) {
        log->info("[mb] found " + std::to_string(candidates.size()) + " linked release(s)");
        for (const auto& c : candidates) {
            log->debug("[mb]   release " + c.id +
                       (c.hasFront ? " (front cover advertised)" : " (no front flag)"));
        }
    }

    for (const auto& c : candidates) {
        if (log) {
            log->info("[mb] trying release MBID: " + c.id);
        }
        if (auto art = fetchCaaFront(curl_, c.id, log)) {
            return art;
        }
    }

    if (log) {
        log->warn("[mb] no Cover Art Archive front image for any linked release");
    }
    return std::nullopt;
}

}  // namespace optigrab
