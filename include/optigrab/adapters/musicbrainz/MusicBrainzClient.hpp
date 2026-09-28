#pragma once

#include "optigrab/ports/ReleaseLookup.hpp"

#include <string>

namespace optigrab {

// MusicBrainz web service (ws/2) over the curl binary.
class MusicBrainzClient : public ReleaseLookup {
public:
    explicit MusicBrainzClient(std::string curlBinary = "curl");

    [[nodiscard]] DiscLookup lookup(const std::string& discId, Logger* log = nullptr) override;
    [[nodiscard]] std::string name() const override { return "musicbrainz"; }

private:
    std::string curl_;
};

}  // namespace optigrab
