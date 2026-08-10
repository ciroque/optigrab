#include "optigrab/cli/Command.hpp"
#include "optigrab/cli/DriveSelection.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/platform/Platform.hpp"

#include <memory>

namespace optigrab {
namespace {

void printField(std::ostream& out, const char* label, const std::string& value, const char* key) {
    out << "  " << label << " : " << value << "  (" << key << ")\n";
}

void printOpt(std::ostream& out, const char* label, const std::optional<std::string>& value,
              const char* key) {
    printField(out, label, value ? *value : std::string("(none)"), key);
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
// Keys in parentheses are the set/flag names that control each field.
class DetailContextCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        const auto& s = ctx.session;

        ctx.out << "Context  (detail context)\n";
        printField(ctx.out, "Platform     ", platformName(), "platform");
        printField(ctx.out, "Exit code    ", std::to_string(ctx.exitCode), "exitcode");
        printField(ctx.out, "Rip active   ", s.ripInProgress() ? "yes" : "no", "rip");

        ctx.out << "Drive  (select drive / --drive)\n";
        if (s.hasSelectedDrive()) {
            const auto& d = s.selectedDrive();
            printField(ctx.out, "Index        ", std::to_string(d.index), "select drive");
            printField(ctx.out, "Path         ", d.path, "select drive / --drive");
            printField(ctx.out, "Model        ", d.model.empty() ? "(unknown)" : d.model,
                       "list drive");
        } else {
            ctx.out << "  (none selected)  (select drive / --drive)\n";
        }

        ctx.out << "Disc  (list track / detail disc)\n";
        if (s.hasDisc()) {
            const auto& disc = s.disc();
            printField(ctx.out, "Device       ", disc.devicePath, "list track");
            printOpt(ctx.out, "Album        ", disc.album, "disc / set album");
            printOpt(ctx.out, "Album artist ", disc.albumArtist, "disc / set artist");
            printField(ctx.out, "Tracks       ", std::to_string(disc.tracks.size()), "list track");
            int audio = 0;
            for (const auto& t : disc.tracks) {
                if (t.audio) {
                    ++audio;
                }
            }
            printField(ctx.out, "Audio tracks ", std::to_string(audio), "list track");
        } else {
            ctx.out << "  (not loaded)  (list track / detail disc)\n";
        }

        ctx.out << "Output\n";
        printField(ctx.out, "Directory    ", s.outputDirectory(), "set out / --out");
        printField(ctx.out, "Folder layout", toString(s.folderLayout()),
                   "set folderlayout / --folder-layout");
        printField(ctx.out, "Quality      ", toString(s.quality()), "set quality / --quality");
        printOpt(ctx.out, "Artist       ", s.artist(), "set artist / --artist");
        printOpt(ctx.out, "Album        ", s.album(), "set album / --album");

        ctx.out << "Backends\n";
        printField(ctx.out, "Extractor    ", toString(s.extractor()),
                   "set extractor / --extractor");
        printField(ctx.out, "Encoder      ", toString(s.encoder()), "set encoder / --encoder");

        ctx.out << "Cover art\n";
        printField(ctx.out, "Fetch/embed  ", s.fetchCoverArt() ? "on" : "off",
                   "set coverart / --no-cover");
        printOpt(ctx.out, "Local cover  ", s.coverPath(), "set cover / --cover");
        printField(ctx.out, "If missing   ", toString(s.coverMissingPolicy()),
                   "set covermissing / --cover-missing");

        ctx.out << "Logging\n";
        printField(ctx.out, "Level        ", toString(ctx.log.level()),
                   "set loglevel / --log-level");
        printOpt(ctx.out, "Log path     ", s.logPathDir(), "set logpath / --log-path");
        if (ctx.log.secondaryFilePath()) {
            printField(ctx.out, "Log file     ", ctx.log.secondaryFilePath()->string(),
                       "logpath → <Artist> - <Album>.log");
        } else {
            printField(ctx.out, "Log file     ", "(none)", "logpath → <Artist> - <Album>.log");
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
