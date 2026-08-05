#pragma once

#include <string>

namespace optigrab {

// True if stdin is an interactive terminal.
[[nodiscard]] bool stdinIsInteractive();

// Ask a yes/no question on out/in. DefaultYes: empty answer → true.
// Returns false if input ends (EOF).
[[nodiscard]] bool promptYesNo(const std::string& question, bool defaultYes = true);

}  // namespace optigrab
