#pragma once

#include "optigrab/log/LogLevel.hpp"

#include <functional>
#include <iostream>
#include <mutex>
#include <string>
#include <string_view>

namespace optigrab {

// Simple leveled logger. Application/user tables stay on Context::out;
// operational messages go through Logger (typically stderr, optional log file).
//
// There is no std::multiwriter (unlike Go's io.MultiWriter). We tee by writing
// the same formatted line to a primary stream and an optional secondary stream.
class Logger {
public:
    using Sink = std::function<void(LogLevel level, std::string_view message)>;

    explicit Logger(std::ostream& stream = std::cerr, LogLevel minLevel = LogLevel::Info);

    void setLevel(LogLevel level);
    [[nodiscard]] LogLevel level() const;

    void setStream(std::ostream& stream);

    // Optional second destination (e.g. log file). nullptr disables.
    // Both primary and secondary receive the same formatted line when no custom sink is set.
    void setSecondaryStream(std::ostream* stream);
    [[nodiscard]] std::ostream* secondaryStream() const;

    // Optional custom sink (tests). When set, replaces stream formatting/tee.
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
    std::ostream* secondary_{nullptr};
    LogLevel minLevel_;
    Sink sink_;
    mutable std::mutex mutex_;
};

}  // namespace optigrab
