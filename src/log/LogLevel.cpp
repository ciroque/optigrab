#include "optigrab/log/LogLevel.hpp"

#include <algorithm>
#include <cctype>

namespace optigrab {

const char* toString(LogLevel level) {
    switch (level) {
    case LogLevel::Trace: return "TRACE";
    case LogLevel::Debug: return "DEBUG";
    case LogLevel::Info: return "INFO";
    case LogLevel::Warn: return "WARN";
    case LogLevel::Error: return "ERROR";
    case LogLevel::Fatal: return "FATAL";
    case LogLevel::Off: return "OFF";
    }
    return "UNKNOWN";
}

std::optional<LogLevel> parseLogLevel(std::string_view text) {
    std::string s(text);
    std::transform(s.begin(), s.end(), s.begin(),
                   [](unsigned char c) { return static_cast<char>(std::tolower(c)); });
    if (s == "trace" || s == "trc") return LogLevel::Trace;
    if (s == "debug" || s == "dbg") return LogLevel::Debug;
    if (s == "info" || s == "information") return LogLevel::Info;
    if (s == "warn" || s == "warning") return LogLevel::Warn;
    if (s == "error" || s == "err") return LogLevel::Error;
    if (s == "fatal" || s == "critical" || s == "crit") return LogLevel::Fatal;
    if (s == "off" || s == "none" || s == "silent") return LogLevel::Off;
    return std::nullopt;
}

}  // namespace optigrab
