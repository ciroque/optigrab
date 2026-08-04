#include "optigrab/cli/Command.hpp"
#include "optigrab/cli/DriveSelection.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/domain/TrackRange.hpp"

#include <memory>
#include <sstream>

namespace optigrab {
namespace {

class RipTrackCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        // rip track <spec>
        if (tokens.size() < 3) {
            throw ParseError("Usage: rip track <all|N|N-M|N,M,...>");
        }
        std::ostringstream spec;
        for (std::size_t i = 2; i < tokens.size(); ++i) {
            if (i > 2) {
                spec << ' ';
            }
            spec << tokens[i];
        }

        ensureDriveSelected(ctx);
        if (!ctx.session.hasDisc()) {
            ctx.ripper->loadDisc(ctx.session, &ctx.log);
        }
        const int maxTrack = static_cast<int>(ctx.session.disc().tracks.size());
        const auto tracks = parseTrackRange(spec.str(), maxTrack);

        const auto results = ctx.ripper->ripTracks(ctx.session, tracks, &ctx.log);

        int ok = 0;
        int fail = 0;
        for (const auto& r : results) {
            if (r.success) {
                ++ok;
                ctx.out << "Track " << r.trackNumber << " -> " << r.outputPath.string() << "\n";
            } else {
                ++fail;
                ctx.log.error("track " + std::to_string(r.trackNumber) + " failed: " + r.message);
            }
        }
        ctx.out << "Done. " << ok << " succeeded, " << fail << " failed.\n";
        if (fail > 0 && ctx.exitCode == 0) {
            ctx.exitCode = 2;
        }
    }
    [[nodiscard]] std::string name() const override { return "rip track"; }
};

}  // namespace

std::unique_ptr<Command> makeRipTrackCommand() { return std::make_unique<RipTrackCommand>(); }

}  // namespace optigrab
