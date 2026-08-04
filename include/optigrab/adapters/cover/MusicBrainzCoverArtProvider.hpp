#pragma once

#include "optigrab/ports/CoverArtProvider.hpp"

#include <string>

namespace optigrab {

class MusicBrainzCoverArtProvider : public CoverArtProvider {
public:
    explicit MusicBrainzCoverArtProvider(std::string curlBinary = "curl");

    [[nodiscard]] std::optional<CoverArt> fetch(const DiscInfo& disc, const Session& session,
                                                LogFn log = {}) override;
    [[nodiscard]] std::string name() const override { return "musicbrainz+caa"; }

private:
    std::string curl_;
};

}  // namespace optigrab
