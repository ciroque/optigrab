#pragma once

#include "optigrab/ports/CoverArtProvider.hpp"
#include "optigrab/ports/ReleaseLookup.hpp"

#include <memory>
#include <string>

namespace optigrab {

// Disc ID → linked releases (via ReleaseLookup) → Cover Art Archive front image.
class MusicBrainzCoverArtProvider : public CoverArtProvider {
public:
    explicit MusicBrainzCoverArtProvider(std::shared_ptr<ReleaseLookup> releases,
                                         std::string curlBinary = "curl");

    [[nodiscard]] std::optional<CoverArt> fetch(const DiscInfo& disc, const Session& session,
                                                Logger* log = nullptr) override;
    [[nodiscard]] std::string name() const override { return "musicbrainz+caa"; }

private:
    std::shared_ptr<ReleaseLookup> releases_;
    std::string curl_;
};

}  // namespace optigrab
