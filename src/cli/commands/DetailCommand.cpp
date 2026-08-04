#include "optigrab/cli/Command.hpp"
#include "optigrab/cli/DriveSelection.hpp"

#include "optigrab/domain/Errors.hpp"

#include <memory>

namespace optigrab {
namespace {

class DetailDriveCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        ensureDriveSelected(ctx);
        const auto& d = ctx.session.selectedDrive();
        ctx.out << "Selected drive\n";
        ctx.out << "  Index : " << d.index << "\n";
        ctx.out << "  Path  : " << d.path << "\n";
        ctx.out << "  Model : " << (d.model.empty() ? "(unknown)" : d.model) << "\n";
        ctx.out << "  Extractor : " << toString(ctx.session.extractor()) << "\n";
        ctx.out << "  Encoder   : " << toString(ctx.session.encoder()) << "\n";
        ctx.out << "  Quality   : " << toString(ctx.session.quality()) << "\n";
        ctx.out << "  Output    : " << ctx.session.outputDirectory() << "\n";
    }
    [[nodiscard]] std::string name() const override { return "detail drive"; }
};

class DetailDiscCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        ensureDriveSelected(ctx);
        if (!ctx.session.hasDisc()) {
            ctx.ripper->loadDisc(ctx.session, [&](const std::string& m) { ctx.err << m << "\n"; });
        }
        const auto& disc = ctx.session.disc();
        ctx.out << "Disc on " << disc.devicePath << "\n";
        if (disc.album) {
            ctx.out << "  Album  : " << *disc.album << "\n";
        }
        if (disc.albumArtist) {
            ctx.out << "  Artist : " << *disc.albumArtist << "\n";
        }
        ctx.out << "  Tracks : " << disc.tracks.size() << "\n";
        int audio = 0;
        for (const auto& t : disc.tracks) {
            if (t.audio) {
                ++audio;
            }
        }
        ctx.out << "  Audio  : " << audio << "\n";
    }
    [[nodiscard]] std::string name() const override { return "detail disc"; }
};

}  // namespace

std::unique_ptr<Command> makeDetailDriveCommand() {
    return std::make_unique<DetailDriveCommand>();
}
std::unique_ptr<Command> makeDetailDiscCommand() { return std::make_unique<DetailDiscCommand>(); }

}  // namespace optigrab
