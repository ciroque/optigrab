#include "optigrab/cli/DriveSelection.hpp"

#include "optigrab/domain/Errors.hpp"

namespace optigrab {

bool tryAutoSelectSingleDrive(Context& ctx, bool announce) {
    if (ctx.session.hasSelectedDrive()) {
        return true;
    }
    if (!ctx.drives) {
        return false;
    }
    const auto drives = ctx.drives->listDrives();
    if (drives.size() != 1) {
        return false;
    }
    ctx.session.selectDrive(drives.front());
    if (announce) {
        const auto& d = drives.front();
        ctx.out << "Drive " << d.index << " is now the selected drive (" << d.path
                << ") [auto].\n";
    }
    return true;
}

void ensureDriveSelected(Context& ctx) {
    if (tryAutoSelectSingleDrive(ctx, true)) {
        return;
    }
    if (!ctx.drives) {
        throw SessionError("No drive enumerator available");
    }
    const auto drives = ctx.drives->listDrives();
    if (drives.empty()) {
        throw SessionError("No optical drives found.");
    }
    throw SessionError(
        "No drive selected. Multiple drives present; use: select drive <index|path>");
}

}  // namespace optigrab
