#include "optigrab/adapters/musicbrainz/MusicBrainzClient.hpp"

#include "optigrab/adapters/musicbrainz/Curl.hpp"
#include "optigrab/adapters/musicbrainz/MusicBrainzJson.hpp"
#include "optigrab/domain/Errors.hpp"

#include <filesystem>

namespace optigrab {

MusicBrainzClient::MusicBrainzClient(std::string curlBinary) : curl_(std::move(curlBinary)) {}

DiscLookup MusicBrainzClient::lookup(const std::string& discId, Logger* log) {
    DiscLookup result;
    result.discId = discId;

    const auto tmp = std::filesystem::temp_directory_path() / ("optigrab-mb-" + discId + ".json");
    // cdstubs=no: stubs carry no release MBID, so they are useless for tags or cover art.
    const std::string url = "https://musicbrainz.org/ws/2/discid/" + discId +
                            "?fmt=json&cdstubs=no&inc=artist-credits+recordings";
    if (log) {
        log->info("[mb] querying MusicBrainz for disc ID " + discId + " ...");
        log->debug("[mb] GET " + url);
    }

    const auto r = curlToFile(curl_, url, tmp, /*strictHttpFail=*/true);
    const auto body = r.fileOk ? readWholeFile(tmp) : std::string();
    std::error_code ec;
    std::filesystem::remove(tmp, ec);

    if (r.exitCode != 0 || !r.fileOk) {
        if (httpStatusFromCurlOutput(r.output) == 404) {
            if (log) {
                log->info("[mb] disc ID not found in MusicBrainz");
            }
            return result;
        }
        logCurlFailure(log, "MusicBrainz discid lookup", url, r);
        throw LookupError("MusicBrainz lookup failed: " + describeCurlFailure(r));
    }

    if (log) {
        log->debug("[mb] MusicBrainz response " + std::to_string(body.size()) + " bytes");
    }
    try {
        result.releases = parseDiscIdReleases(body, discId);
    } catch (const OptigrabError& ex) {
        if (log) {
            log->debug("[mb] snippet: " + trimSnippet(body));
        }
        throw LookupError(ex.what());
    }
    if (log) {
        log->info("[mb] found " + std::to_string(result.releases.size()) + " linked release(s)");
    }
    return result;
}

}  // namespace optigrab
