#include "optigrab/cli/Command.hpp"
#include "optigrab/cli/DriveSelection.hpp"

#include "optigrab/domain/Errors.hpp"

#include <memory>

namespace optigrab {
namespace {

class EjectDriveCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (!ctx.ejector) {
            throw DriveError("Eject is not available in this build");
        }

        DriveInfo target;
        if (tokens.size() >= 3) {
            const std::string& arg = tokens[2];
            const auto drives = ctx.drives->listDrives();
            if (drives.empty()) {
                throw DriveError("No optical drives found");
            }
            const DriveInfo* chosen = nullptr;
            try {
                const int idx = std::stoi(arg);
                for (const auto& d : drives) {
                    if (d.index == idx) {
                        chosen = &d;
                        break;
                    }
                }
            } catch (const std::exception&) {
            }
            if (!chosen) {
                for (const auto& d : drives) {
                    if (d.path == arg) {
                        chosen = &d;
                        break;
                    }
                }
            }
            if (!chosen) {
                throw DriveError("Drive not found: " + arg);
            }
            target = *chosen;
        } else {
            ensureDriveSelected(ctx);
            target = ctx.session.selectedDrive();
        }

        if (ctx.session.ripInProgress()) {
            throw SessionError("Cannot eject while a rip is in progress");
        }

        ctx.log.info("ejecting " + target.path);
        ctx.ejector->eject(target.path);
        ctx.session.clearDisc();
        ctx.out << "Ejected " << target.path << "\n";
        ctx.log.info("ejected " + target.path);
    }
    [[nodiscard]] std::string name() const override { return "eject drive"; }
};

}  // namespace

std::unique_ptr<Command> makeEjectDriveCommand() {
    return std::make_unique<EjectDriveCommand>();
}

}  // namespace optigrab
