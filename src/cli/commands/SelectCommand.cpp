#include "optigrab/cli/Command.hpp"

#include "optigrab/domain/Errors.hpp"

#include <memory>
#include <stdexcept>

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

// select release <n> — pick among releases found by `lookup disc`.
class SelectReleaseCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (tokens.size() != 3) {
            throw ParseError("Usage: select release <n>  (see: lookup disc)");
        }
        std::size_t idx = 0;
        try {
            std::size_t used = 0;
            const long long n = std::stoll(tokens[2], &used);
            if (used != tokens[2].size() || n < 0) {
                throw std::invalid_argument("negative");
            }
            idx = static_cast<std::size_t>(n);
        } catch (const std::exception&) {
            throw ParseError("Release must be a number from lookup disc: " + tokens[2]);
        }

        ctx.session.selectRelease(idx);
        const auto* r = ctx.session.selectedRelease();
        ctx.out << "Release " << idx << " is now the selected release (" << r->artist << " - "
                << r->title;
        if (!r->date.empty()) {
            ctx.out << ", " << r->date;
        }
        if (!r->country.empty()) {
            ctx.out << ", " << r->country;
        }
        ctx.out << ").\n";
    }
    [[nodiscard]] std::string name() const override { return "select release"; }
};

}  // namespace

std::unique_ptr<Command> makeSelectDriveCommand() {
    return std::make_unique<SelectDriveCommand>();
}
std::unique_ptr<Command> makeSelectReleaseCommand() {
    return std::make_unique<SelectReleaseCommand>();
}

}  // namespace optigrab
