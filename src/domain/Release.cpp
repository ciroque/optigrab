#include "optigrab/domain/Release.hpp"

#include <cctype>

namespace optigrab {

std::optional<int> Release::year() const {
    if (date.size() < 4) {
        return std::nullopt;
    }
    int y = 0;
    for (std::size_t i = 0; i < 4; ++i) {
        const auto c = static_cast<unsigned char>(date[i]);
        if (!std::isdigit(c)) {
            return std::nullopt;
        }
        y = y * 10 + (c - '0');
    }
    return y;
}

bool applyRelease(DiscInfo& disc, const Release& release) {
    std::vector<TrackInfo*> audio;
    for (auto& t : disc.tracks) {
        if (t.audio) {
            audio.push_back(&t);
        }
    }
    if (audio.size() != release.tracks.size()) {
        return false;
    }

    for (std::size_t i = 0; i < audio.size(); ++i) {
        const auto& rt = release.tracks[i];
        if (!rt.title.empty()) {
            audio[i]->title = rt.title;
        }
        audio[i]->artist = rt.artist.empty() ? release.artist : rt.artist;
    }
    if (!release.title.empty()) {
        disc.album = release.title;
    }
    if (!release.artist.empty()) {
        disc.albumArtist = release.artist;
    }
    disc.year = release.year();
    return true;
}

}  // namespace optigrab
