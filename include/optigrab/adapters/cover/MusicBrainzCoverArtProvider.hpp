#pragma once

#include "optigrab/ports/CoverArtProvider.hpp"

#include <string>

namespace optigrab {

// MusicBrainz disc ID → Cover Art Archive front image via curl.
class MusicBrainzCoverArtProvider : public CoverArtProvider {
public:
    explicit MusicBrainzCoverArtProvider(std::string curlBinary = "curl");

    [[nodiscard]] std::optional<CoverArt> fetch(const DiscInfo& disc,
                                                const Session& session) override;
    [[nodiscard]] std::string name() const override { return "musicbrainz+caa"; }

private:
    std::string curl_;
};

}  // namespace optigrab
