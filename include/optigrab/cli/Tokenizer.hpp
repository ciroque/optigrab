#pragma once

#include <string>
#include <vector>

namespace optigrab {

// Split a command line into tokens. Supports double-quoted strings.
// Throws ParseError on unbalanced quotes.
[[nodiscard]] std::vector<std::string> tokenize(const std::string& input);

}  // namespace optigrab
