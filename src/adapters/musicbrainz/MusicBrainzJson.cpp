#include "optigrab/adapters/musicbrainz/MusicBrainzJson.hpp"

#include "optigrab/domain/Errors.hpp"

#include <nlohmann/json.hpp>

namespace optigrab {
namespace {

using nlohmann::json;

// MusicBrainz uses null for unknown date/country/barcode; treat missing and null alike.
std::string str(const json& obj, const char* key) {
    const auto it = obj.find(key);
    if (it == obj.end() || !it->is_string()) {
        return {};
    }
    return it->get<std::string>();
}

int integer(const json& obj, const char* key) {
    const auto it = obj.find(key);
    if (it == obj.end() || !it->is_number_integer()) {
        return 0;
    }
    return it->get<int>();
}

// "artist-credit": [{"name": "A", "joinphrase": " feat. "}, {"name": "B", ...}] → "A feat. B"
std::string artistCredit(const json& obj) {
    const auto it = obj.find("artist-credit");
    if (it == obj.end() || !it->is_array()) {
        return {};
    }
    std::string out;
    for (const auto& credit : *it) {
        if (!credit.is_object()) {
            continue;
        }
        out += str(credit, "name");
        out += str(credit, "joinphrase");
    }
    return out;
}

bool mediumHasDisc(const json& medium, const std::string& discId) {
    const auto discs = medium.find("discs");
    if (discs == medium.end() || !discs->is_array()) {
        return false;
    }
    for (const auto& d : *discs) {
        if (d.is_object() && str(d, "id") == discId) {
            return true;
        }
    }
    return false;
}

// Medium carrying discId; else the only medium; else none (can't tell which disc this is).
const json* findMedium(const json& release, const std::string& discId) {
    const auto media = release.find("media");
    if (media == release.end() || !media->is_array() || media->empty()) {
        return nullptr;
    }
    for (const auto& m : *media) {
        if (m.is_object() && mediumHasDisc(m, discId)) {
            return &m;
        }
    }
    if (media->size() == 1 && media->front().is_object()) {
        return &media->front();
    }
    return nullptr;
}

Release parseRelease(const json& r, const std::string& discId) {
    Release rel;
    rel.id = str(r, "id");
    rel.title = str(r, "title");
    rel.artist = artistCredit(r);
    rel.date = str(r, "date");
    rel.country = str(r, "country");
    rel.barcode = str(r, "barcode");

    const auto caa = r.find("cover-art-archive");
    if (caa != r.end() && caa->is_object()) {
        const auto front = caa->find("front");
        rel.hasFrontCover = front != caa->end() && front->is_boolean() && front->get<bool>();
    }

    const auto media = r.find("media");
    if (media != r.end() && media->is_array()) {
        rel.mediumCount = static_cast<int>(media->size());
    }

    if (const json* medium = findMedium(r, discId)) {
        rel.mediumPosition = integer(*medium, "position");
        const auto tracks = medium->find("tracks");
        if (tracks != medium->end() && tracks->is_array()) {
            for (const auto& t : *tracks) {
                if (!t.is_object()) {
                    continue;
                }
                ReleaseTrack rt;
                rt.position = integer(t, "position");
                rt.title = str(t, "title");
                rt.artist = artistCredit(t);
                rel.tracks.push_back(std::move(rt));
            }
        }
    }
    return rel;
}

}  // namespace

std::vector<Release> parseDiscIdReleases(const std::string& body, const std::string& discId) {
    const json doc = json::parse(body, nullptr, /*allow_exceptions=*/false);
    if (doc.is_discarded()) {
        throw OptigrabError("MusicBrainz response is not valid JSON");
    }

    std::vector<Release> out;
    if (!doc.is_object()) {
        return out;
    }
    const auto releases = doc.find("releases");
    if (releases == doc.end() || !releases->is_array()) {
        return out;  // unknown disc, or a CD stub (no linked releases)
    }
    for (const auto& r : *releases) {
        if (!r.is_object()) {
            continue;
        }
        auto rel = parseRelease(r, discId);
        if (!rel.id.empty()) {
            out.push_back(std::move(rel));
        }
    }
    return out;
}

}  // namespace optigrab
