#include "optigrab/cli/Command.hpp"

#include "optigrab/domain/Errors.hpp"

#include <memory>

namespace optigrab {
namespace {

class SelectDriveCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        // tokens: select drive <arg>
        if (tokens.size() < 3) {
            throw ParseError("Usage: select drive <index|path>");
        }
        const std::string& arg = tokens[2];
        const auto drives = ctx.drives->listDrives();
        if (drives.empty()) {
            throw DriveError("No optical drives found");
        }

        const DriveInfo* chosen = nullptr;
        // Try index
        try {
            const int idx = std::stoi(arg);
            for (const auto& d : drives) {
                if (d.index == idx) {
                    chosen = &d;
                    break;
                }
            }
        } catch (const std::exception&) {
            // not an index
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

        ctx.session.selectDrive(*chosen);
        ctx.out << "Drive " << chosen->index << " is now the selected drive (" << chosen->path
                << ").\n";
    }
    [[nodiscard]] std::string name() const override { return "select drive"; }
};

}  // namespace

std::unique_ptr<Command> makeSelectDriveCommand() {
    return std::make_unique<SelectDriveCommand>();
}

}  // namespace optigrab
