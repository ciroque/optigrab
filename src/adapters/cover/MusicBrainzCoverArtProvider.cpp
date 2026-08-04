#include "optigrab/adapters/cover/MusicBrainzCoverArtProvider.hpp"

#include "optigrab/domain/DiscId.hpp"
#include "optigrab/util/Process.hpp"

#include <filesystem>
#include <fstream>
#include <regex>

namespace optigrab {
namespace {

constexpr const char* kUserAgent = "optigrab/0.2.0 (https://github.com/ciroque/optigrab)";

bool curlToFile(const std::string& curlBin, const std::string& url,
                const std::filesystem::path& outFile) {
    const std::vector<std::string> args = {
        curlBin, "-fsSL", "--max-time", "30", "-A", kUserAgent, "-o", outFile.string(), url,
    };
    std::string out;
    std::string err;
    return runProcess(args, out, err) == 0 && std::filesystem::exists(outFile) &&
           std::filesystem::file_size(outFile) > 0;
}

std::string readFile(const std::filesystem::path& p) {
    std::ifstream in(p, std::ios::binary);
    return std::string(std::istreambuf_iterator<char>(in), std::istreambuf_iterator<char>());
}

// Very small JSON probe: first "id":"<uuid>" inside a "releases" section is good enough
// for Cover Art Archive when the discid lookup returns releases.
std::optional<std::string> firstReleaseId(const std::string& json) {
    // Prefer releases array entries.
    const auto releasesPos = json.find("\"releases\"");
    const std::string scope =
        releasesPos == std::string::npos ? json : json.substr(releasesPos);
    static const std::regex re(
        "\"id\"\\s*:\\s*\"([0-9a-fA-F]{8}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{4}-[0-9a-fA-F]{12})\"");
    std::smatch m;
    if (std::regex_search(scope, m, re) && m.size() >= 2) {
        return m[1].str();
    }
    return std::nullopt;
}

}  // namespace

MusicBrainzCoverArtProvider::MusicBrainzCoverArtProvider(std::string curlBinary)
    : curl_(std::move(curlBinary)) {}

std::optional<CoverArt> MusicBrainzCoverArtProvider::fetch(const DiscInfo& disc, const Session&) {
    const auto discId = computeMusicBrainzDiscId(disc);
    if (!discId) {
        return std::nullopt;
    }

    const auto tmp = std::filesystem::temp_directory_path() / ("optigrab-mb-" + *discId + ".json");
    const std::string mbUrl =
        "https://musicbrainz.org/ws/2/discid/" + *discId + "?fmt=json&inc=artists";

    if (!curlToFile(curl_, mbUrl, tmp)) {
        std::error_code ec;
        std::filesystem::remove(tmp, ec);
        return std::nullopt;
    }

    const auto json = readFile(tmp);
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    const auto releaseId = firstReleaseId(json);
    if (!releaseId) {
        return std::nullopt;
    }

    const auto imgPath =
        std::filesystem::temp_directory_path() / ("optigrab-cover-" + *releaseId + ".img");
    // Prefer a reasonably sized front image; CAA redirects to the real file.
    const std::string caaUrl = "https://coverartarchive.org/release/" + *releaseId + "/front-500";
    if (!curlToFile(curl_, caaUrl, imgPath)) {
        // Fallback to full front
        const std::string caaFront = "https://coverartarchive.org/release/" + *releaseId + "/front";
        if (!curlToFile(curl_, caaFront, imgPath)) {
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
    if (art.bytes.empty() || (!isJpeg(art.bytes) && !isPng(art.bytes))) {
        return std::nullopt;
    }
    art.mimeType = guessMime(art.bytes);
    art.source = "coverartarchive:" + *releaseId;
    return art;
}

}  // namespace optigrab
