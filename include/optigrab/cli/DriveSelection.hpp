#pragma once

#include "optigrab/cli/Context.hpp"

namespace optigrab {

// If no drive is selected and exactly one optical drive exists, select it.
// Prints a short notice when announce is true. Returns true if a drive is selected
// afterward (already selected or auto-selected).
bool tryAutoSelectSingleDrive(Context& ctx, bool announce = true);

// Ensures a drive is selected: auto-selects when only one is present, otherwise
// throws SessionError with a clear message (none / multiple).
void ensureDriveSelected(Context& ctx);

}  // namespace optigrab
