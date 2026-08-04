#pragma once

#include <string>
#include <vector>

namespace optigrab {

// Parses: "all" | "3" | "1-3" | "1,3,5" | "1-3,5"
// Returns 1-based track numbers sorted unique.
// Throws ParseError on invalid input.
[[nodiscard]] std::vector<int> parseTrackRange(const std::string& spec, int maxTrack);

}  // namespace optigrab
