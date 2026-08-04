#pragma once

#include "optigrab/cli/History.hpp"

#include <iostream>
#include <optional>
#include <string>

namespace optigrab {

// Reads a single line from stdin with optional history navigation.
// When stdin is a TTY: raw mode, up/down arrows, backspace.
// When not a TTY: falls back to std::getline (pipes/scripts).
// Returns nullopt on EOF.
class LineReader {
public:
    explicit LineReader(History& history, std::ostream& out = std::cout,
                        std::istream& in = std::cin);

    [[nodiscard]] std::optional<std::string> readLine(const std::string& prompt);

private:
    [[nodiscard]] std::optional<std::string> readLineInteractive(const std::string& prompt);
    [[nodiscard]] std::optional<std::string> readLinePlain();

    History& history_;
    std::ostream& out_;
    std::istream& in_;
};

}  // namespace optigrab
