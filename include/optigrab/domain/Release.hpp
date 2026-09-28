#pragma once

#include <optional>
#include <string>
#include <vector>

namespace optigrab {

// One track on the medium that matched the disc (MusicBrainz "track", not "recording").
struct ReleaseTrack {
    int position{0};     // 1-based position on the medium
    std::string title;
    std::string artist;  // flattened artist credit, e.g. "A feat. B"
};

// A MusicBrainz release linked to a Disc ID, reduced to what optigrab needs.
struct Release {
    std::string id;       // release MBID
    std::string title;
    std::string artist;   // flattened release artist credit
    std::string date;     // "YYYY", "YYYY-MM" or "YYYY-MM-DD"; may be empty
    std::string country;  // ISO code or "XW"/"XE"; may be empty
    std::string barcode;
    bool hasFrontCover{false};  // Cover Art Archive advertises a front image
    int mediumPosition{0};      // which disc in a multi-disc set (1-based; 0 if unknown)
    int mediumCount{0};
    std::vector<ReleaseTrack> tracks;  // tracks of the matching medium only

    // Leading 4-digit year of date, if present.
    [[nodiscard]] std::optional<int> year() const;
};

// Result of a Disc ID lookup. Empty releases = disc not in MusicBrainz.
struct DiscLookup {
    std::string discId;
    std::vector<Release> releases;
};

}  // namespace optigrab
