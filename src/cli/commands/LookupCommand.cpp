#include "optigrab/cli/Command.hpp"
#include "optigrab/cli/DriveSelection.hpp"

#include "optigrab/domain/DiscId.hpp"
#include "optigrab/domain/Errors.hpp"

#include <algorithm>
#include <cctype>
#include <iomanip>
#include <memory>
#include <optional>
#include <string>

namespace optigrab {
namespace {

// 28 chars of the MusicBrainz Base64 alphabet (A-Z a-z 0-9 . _ -).
bool looksLikeDiscId(const std::string& s) {
    return s.size() == 28 && std::all_of(s.begin(), s.end(), [](char c) {
               return std::isalnum(static_cast<unsigned char>(c)) || c == '.' || c == '_' ||
                      c == '-';
           });
}

std::string orDash(const std::string& s) { return s.empty() ? "-" : s; }

std::string mediumLabel(const Release& r) {
    if (r.mediumPosition == 0) {
        return "?/" + std::to_string(r.mediumCount);
    }
    return std::to_string(r.mediumPosition) + "/" + std::to_string(r.mediumCount);
}

// selected: marked with "*" and its track list shown (release 0's if none).
void printReleases(Context& ctx, const DiscLookup& found, std::optional<std::size_t> selected) {
    ctx.out << "Releases : " << found.releases.size() << "\n";
    if (found.releases.empty()) {
        return;
    }

    std::size_t wName = 7;  // "Release"
    for (const auto& r : found.releases) {
        wName = std::max(wName, (r.artist + " - " + r.title).size());
    }
    wName = std::min<std::size_t>(wName, 48);

    ctx.out << "\n"
            << std::left << "    " << std::setw(4) << "#" << std::setw(static_cast<int>(wName) + 2)
            << "Release" << std::setw(12) << "Date" << std::setw(9) << "Country"
            << std::setw(7) << "Disc" << "Cover\n";
    for (std::size_t i = 0; i < found.releases.size(); ++i) {
        const auto& r = found.releases[i];
        auto name = r.artist + " - " + r.title;
        if (name.size() > wName) {
            name = name.substr(0, wName - 3) + "...";
        }
        ctx.out << (selected == i ? "  * " : "    ") << std::setw(4) << i
                << std::setw(static_cast<int>(wName) + 2) << name << std::setw(12) << orDash(r.date) << std::setw(9) << orDash(r.country)
                << std::setw(7) << mediumLabel(r) << (r.hasFrontCover ? "yes" : "no") << "\n";
    }

    const auto shownIndex = selected.value_or(0);
    const auto& shown = found.releases[shownIndex];
    ctx.out << "\nTracks (release " << shownIndex << ", MBID " << shown.id << "):\n";
    if (shown.tracks.empty()) {
        ctx.out << "  (no track list for this disc)\n";
        return;
    }
    for (const auto& t : shown.tracks) {
        ctx.out << "  " << std::right << std::setw(2) << t.position << std::left << "  "
                << t.title;
        if (!t.artist.empty() && t.artist != shown.artist) {
            ctx.out << " (" << t.artist << ")";
        }
        ctx.out << "\n";
    }
}

// lookup disc [discid] — query MusicBrainz without ripping.
// For the loaded disc the result is kept in the session (release 0 selected) so a later
// rip reuses it; an explicit disc ID is display-only.
class LookupDiscCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (!ctx.releases) {
            throw SessionError("MusicBrainz lookup is not configured");
        }
        if (tokens.size() > 3) {
            throw ParseError("Usage: lookup disc [discid]");
        }

        std::string discId;
        std::optional<std::string> submitUrl;
        bool loadedNow = false;
        if (tokens.size() == 3) {
            discId = tokens[2];
            if (!looksLikeDiscId(discId)) {
                throw ParseError("Not a MusicBrainz disc ID (28 chars of A-Z a-z 0-9 . _ -): " +
                                 discId);
            }
        } else {
            ensureDriveSelected(ctx);
            if (!ctx.session.hasDisc()) {
                ctx.ripper->loadDisc(ctx.session, &ctx.log);  // may look up MusicBrainz itself
                loadedNow = true;
            }
            const auto& disc = ctx.session.disc();
            const auto id = computeMusicBrainzDiscId(disc);
            if (!id) {
                throw TocError("Cannot compute a MusicBrainz disc ID (no audio tracks?)");
            }
            discId = *id;
            submitUrl = musicBrainzSubmitUrl(disc);
        }

        const bool forLoadedDisc = tokens.size() == 2;
        // Reuse what loading the disc just fetched; otherwise always ask MusicBrainz afresh.
        const auto& cached = ctx.session.discLookup();
        const bool reuse = loadedNow && cached && cached->discId == discId;
        auto found = reuse ? *cached : ctx.releases->lookup(discId, &ctx.log);

        ctx.out << "Disc ID  : " << discId << "\n";
        std::optional<std::size_t> selected;
        if (forLoadedDisc && !found.releases.empty()) {
            selected = 0;  // matches Session::setDiscLookup
        }
        printReleases(ctx, found, selected);
        if (found.releases.empty()) {
            ctx.out << "Not in MusicBrainz yet.";
            if (submitUrl) {
                ctx.out << " Add it at:\n  " << *submitUrl;
            }
            ctx.out << "\n";
        } else if (forLoadedDisc && found.releases.size() > 1) {
            ctx.out << "\n* = selected. Choose another with: select release <#>\n";
        }
        if (forLoadedDisc && !reuse) {
            const bool any = !found.releases.empty();
            ctx.session.setDiscLookup(std::move(found));
            if (any) {
                ctx.ripper->loadDisc(ctx.session, &ctx.log);  // apply titles from release 0
            }
        }
    }
    [[nodiscard]] std::string name() const override { return "lookup disc"; }
};

}  // namespace

std::unique_ptr<Command> makeLookupDiscCommand() { return std::make_unique<LookupDiscCommand>(); }

}  // namespace optigrab
