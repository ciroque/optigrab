#include "optigrab/cli/Command.hpp"
#include "optigrab/cli/DriveSelection.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/platform/Platform.hpp"

#include <iomanip>
#include <memory>
#include <vector>

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

// Full session/context dump as a table — does not require a drive or load a disc.
class DetailContextCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        const auto& s = ctx.session;

        struct Row {
            std::string key;
            std::string value;
            std::string via;
        };
        std::vector<Row> rows;

        auto add = [&](std::string key, std::string value, std::string via) {
            rows.push_back(Row{std::move(key), std::move(value), std::move(via)});
        };
        auto opt = [](const std::optional<std::string>& v) -> std::string {
            return v ? *v : std::string("-");
        };

        add("platform", platformName(), "-");
        add("exitcode", std::to_string(ctx.exitCode), "-");
        add("rip", s.ripInProgress() ? "yes" : "no", "rip track");

        if (s.hasSelectedDrive()) {
            const auto& d = s.selectedDrive();
            add("drive", d.path, "select drive / --drive");
            add("drive.index", std::to_string(d.index), "select drive / --drive");
            add("drive.model", d.model.empty() ? "-" : d.model, "list drive");
        } else {
            add("drive", "-", "select drive / --drive");
        }

        if (s.hasDisc()) {
            const auto& disc = s.disc();
            add("disc.device", disc.devicePath, "list track");
            add("disc.album", opt(disc.album), "disc / set album");
            add("disc.artist", opt(disc.albumArtist), "disc / set artist");
            add("disc.tracks", std::to_string(disc.tracks.size()), "list track");
            int audio = 0;
            for (const auto& t : disc.tracks) {
                if (t.audio) {
                    ++audio;
                }
            }
            add("disc.audio", std::to_string(audio), "list track");
        } else {
            add("disc", "-", "list track / detail disc");
        }

        add("out", s.outputDirectory(), "set out / --out");
        add("folderlayout", toString(s.folderLayout()), "set folderlayout / --folder-layout");
        add("quality", toString(s.quality()), "set quality / --quality");
        add("artist", opt(s.artist()), "set artist / --artist");
        add("album", opt(s.album()), "set album / --album");
        add("extractor", toString(s.extractor()), "set extractor / --extractor");
        add("encoder", toString(s.encoder()), "set encoder / --encoder");
        add("coverart", s.fetchCoverArt() ? "on" : "off", "set coverart / --no-cover");
        add("cover", opt(s.coverPath()), "set cover / --cover");
        add("covermissing", toString(s.coverMissingPolicy()),
            "set covermissing / --cover-missing");
        add("loglevel", toString(ctx.log.level()), "set loglevel / --log-level");
        add("logpath", opt(s.logPathDir()), "set logpath / --log-path");
        add("logfile",
            ctx.log.secondaryFilePath() ? ctx.log.secondaryFilePath()->string() : std::string("-"),
            "<Artist> - <Album>.log");

        // Column widths from content (capped so long paths don't blow up the table).
        constexpr std::size_t kMaxValue = 48;
        std::size_t wKey = 3;    // "Key"
        std::size_t wValue = 5;  // "Value"
        std::size_t wVia = 3;    // "Via"
        for (const auto& r : rows) {
            wKey = std::max(wKey, r.key.size());
            wValue = std::max(wValue, std::min(r.value.size(), kMaxValue));
            wVia = std::max(wVia, r.via.size());
        }

        auto trunc = [&](std::string s, std::size_t w) {
            if (s.size() > w) {
                if (w <= 3) {
                    return s.substr(0, w);
                }
                return s.substr(0, w - 3) + "...";
            }
            return s;
        };

        auto rule = [&]() {
            ctx.out << std::string(wKey, '-') << "  " << std::string(wValue, '-') << "  "
                    << std::string(wVia, '-') << "\n";
        };

        ctx.out << std::left << std::setw(static_cast<int>(wKey)) << "Key" << "  "
                << std::setw(static_cast<int>(wValue)) << "Value" << "  "
                << "Via"
                << "\n";
        rule();
        for (const auto& r : rows) {
            ctx.out << std::left << std::setw(static_cast<int>(wKey)) << r.key << "  "
                    << std::setw(static_cast<int>(wValue)) << trunc(r.value, kMaxValue) << "  "
                    << r.via << "\n";
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
