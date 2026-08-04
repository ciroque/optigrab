#pragma once

#include <optional>
#include <string>
#include <string_view>

namespace optigrab {

enum class LogLevel {
    Trace = 0,
    Debug = 1,
    Info = 2,
    Warn = 3,
    Error = 4,
    Fatal = 5,
    Off = 6,
};

[[nodiscard]] const char* toString(LogLevel level);
[[nodiscard]] std::optional<LogLevel> parseLogLevel(std::string_view text);

}  // namespace optigrab
