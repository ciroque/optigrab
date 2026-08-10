#include "optigrab/cli/Command.hpp"
#include "optigrab/cli/DriveSelection.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/platform/Platform.hpp"

#include <memory>

namespace optigrab {
namespace {

void printOpt(std::ostream& out, const char* label, const std::optional<std::string>& value) {
    out << "  " << label << " : " << (value ? *value : std::string("(none)")) << "\n";
}

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
        ctx.out << "  Layout    : " << toString(ctx.session.folderLayout()) << "\n";
        ctx.out << "  Output    : " << ctx.session.outputDirectory() << "\n";
        ctx.out << "  Log path  : "
                << (ctx.logPathDir() ? *ctx.logPathDir() : std::string("(none)")) << "\n";
        if (ctx.log.secondaryFilePath()) {
            ctx.out << "  Log file  : " << ctx.log.secondaryFilePath()->string() << "\n";
        }
    }
    [[nodiscard]] std::string name() const override { return "detail drive"; }
};

class DetailDiscCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        ensureDriveSelected(ctx);
        if (!ctx.session.hasDisc()) {
            ctx.ripper->loadDisc(ctx.session, &ctx.log);
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

// Full session/context dump — does not require a drive or load a disc.
class DetailContextCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        const auto& s = ctx.session;

        ctx.out << "Context\n";
        ctx.out << "  Platform     : " << platformName() << "\n";
        ctx.out << "  Exit code    : " << ctx.exitCode << "\n";
        ctx.out << "  Rip active   : " << (s.ripInProgress() ? "yes" : "no") << "\n";

        ctx.out << "Drive\n";
        if (s.hasSelectedDrive()) {
            const auto& d = s.selectedDrive();
            ctx.out << "  Index        : " << d.index << "\n";
            ctx.out << "  Path         : " << d.path << "\n";
            ctx.out << "  Model        : " << (d.model.empty() ? "(unknown)" : d.model) << "\n";
        } else {
            ctx.out << "  (none selected)\n";
        }

        ctx.out << "Disc\n";
        if (s.hasDisc()) {
            const auto& disc = s.disc();
            ctx.out << "  Device       : " << disc.devicePath << "\n";
            printOpt(ctx.out, "Album       ", disc.album);
            printOpt(ctx.out, "Album artist", disc.albumArtist);
            ctx.out << "  Tracks       : " << disc.tracks.size() << "\n";
            int audio = 0;
            for (const auto& t : disc.tracks) {
                if (t.audio) {
                    ++audio;
                }
            }
            ctx.out << "  Audio tracks : " << audio << "\n";
        } else {
            ctx.out << "  (not loaded — use: list track or detail disc)\n";
        }

        ctx.out << "Output\n";
        ctx.out << "  Directory    : " << s.outputDirectory() << "\n";
        ctx.out << "  Folder layout: " << toString(s.folderLayout()) << "\n";
        ctx.out << "  Quality      : " << toString(s.quality()) << "\n";
        printOpt(ctx.out, "Artist       ", s.artist());
        printOpt(ctx.out, "Album        ", s.album());

        ctx.out << "Backends\n";
        ctx.out << "  Extractor    : " << toString(s.extractor()) << "\n";
        ctx.out << "  Encoder      : " << toString(s.encoder()) << "\n";

        ctx.out << "Cover art\n";
        ctx.out << "  Fetch/embed  : " << (s.fetchCoverArt() ? "on" : "off") << "\n";
        printOpt(ctx.out, "Local cover  ", s.coverPath());
        ctx.out << "  If missing   : " << toString(s.coverMissingPolicy()) << "\n";

        ctx.out << "Logging\n";
        ctx.out << "  Level        : " << toString(ctx.log.level()) << "\n";
        printOpt(ctx.out, "Log path     ", s.logPathDir());
        if (ctx.log.secondaryFilePath()) {
            ctx.out << "  Log file     : " << ctx.log.secondaryFilePath()->string() << "\n";
        } else {
            ctx.out << "  Log file     : (none)\n";
        }
    }
    [[nodiscard]] std::string name() const override { return "detail context"; }
};

}  // namespace

std::unique_ptr<Command> makeDetailDriveCommand() {
    return std::make_unique<DetailDriveCommand>();
}
std::unique_ptr<Command> makeDetailDiscCommand() { return std::make_unique<DetailDiscCommand>(); }
std::unique_ptr<Command> makeDetailContextCommand() {
    return std::make_unique<DetailContextCommand>();
}

}  // namespace optigrab
