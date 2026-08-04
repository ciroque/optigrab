#include "optigrab/cli/Command.hpp"
#include "optigrab/cli/DriveSelection.hpp"

#include "optigrab/domain/Errors.hpp"

#include <iomanip>
#include <memory>
#include <sstream>

namespace optigrab {
namespace {

class ListDriveCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        const auto drives = ctx.drives->listDrives();
        if (drives.empty()) {
            ctx.out << "No optical drives found.\n";
            return;
        }
        ctx.out << std::left << std::setw(6) << "Index" << std::setw(14) << "Path"
                << "Model\n";
        for (const auto& d : drives) {
            ctx.out << std::left << std::setw(6) << d.index << std::setw(14) << d.path
                    << (d.model.empty() ? "(unknown)" : d.model) << "\n";
        }
    }
    [[nodiscard]] std::string name() const override { return "list drive"; }
};

class ListTrackCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        ensureDriveSelected(ctx);
        ctx.ripper->loadDisc(ctx.session, [&](const std::string& m) { ctx.err << m << "\n"; });
        const auto& disc = ctx.session.disc();
        if (disc.tracks.empty()) {
            ctx.out << "No tracks on disc.\n";
            return;
        }
        if (disc.album) {
            ctx.out << "Album: " << *disc.album << "\n";
        }
        ctx.out << std::left << std::setw(4) << "#" << std::setw(8) << "Sectors"
                << "Title\n";
        for (const auto& t : disc.tracks) {
            std::ostringstream title;
            if (!t.audio) {
                title << "[DATA]";
            } else if (!t.title.empty()) {
                title << t.title;
            } else {
                title << "(untitled)";
            }
            ctx.out << std::left << std::setw(4) << t.number << std::setw(8) << t.sectors
                    << title.str() << "\n";
        }
    }
    [[nodiscard]] std::string name() const override { return "list track"; }
};

}  // namespace

std::unique_ptr<Command> makeListDriveCommand() { return std::make_unique<ListDriveCommand>(); }
std::unique_ptr<Command> makeListTrackCommand() { return std::make_unique<ListTrackCommand>(); }

}  // namespace optigrab
