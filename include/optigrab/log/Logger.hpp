#pragma once

#include "optigrab/log/LogLevel.hpp"

#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

namespace optigrab {

// Simple leveled logger. Application/user tables stay on Context::out;
// operational messages go through Logger (typically stderr).
class Logger {
public:
    using Sink = std::function<void(LogLevel level, std::string_view message)>;

    explicit Logger(std::ostream& stream = std::cerr, LogLevel minLevel = LogLevel::Info);

    void setLevel(LogLevel level);
    [[nodiscard]] LogLevel level() const;

    void setStream(std::ostream& stream);

    // Optional custom sink (tests). When set, replaces stream formatting.
    void setSink(Sink sink);
    void clearSink();

    void log(LogLevel level, std::string_view message);
    void trace(std::string_view message);
    void debug(std::string_view message);
    void info(std::string_view message);
    void warn(std::string_view message);
    void error(std::string_view message);
    void fatal(std::string_view message);

    [[nodiscard]] bool enabled(LogLevel level) const;

    // Adapter for older string-only callbacks (emits at the given level).
    [[nodiscard]] std::function<void(const std::string&)> callback(LogLevel level);

private:
    void emit(LogLevel level, std::string_view message);

    std::ostream* stream_;
    LogLevel minLevel_;
    Sink sink_;
    mutable std::mutex mutex_;
};

}  // namespace optigrab
