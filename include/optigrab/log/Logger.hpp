#pragma once

#include "optigrab/log/LogLevel.hpp"

#include <filesystem>
#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <mutex>
#include <optional>
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
    // Clears any owned secondary file. Prefer setSecondaryFile for paths.
    void setSecondaryStream(std::ostream* stream);
    [[nodiscard]] std::ostream* secondaryStream() const;

    // Open path for append, own the ofstream, and tee to it. Creates parent dirs.
    // Throws std::runtime_error (or OptigrabError from callers) — returns false on open fail
    // if you use tryOpenSecondaryFile.
    void setSecondaryFile(const std::filesystem::path& path);
    [[nodiscard]] bool tryOpenSecondaryFile(const std::filesystem::path& path, std::string& errorOut);
    void clearSecondaryFile();
    [[nodiscard]] const std::optional<std::filesystem::path>& secondaryFilePath() const;

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
    void clearOwnedSecondaryUnlocked();

    std::ostream* stream_;
    std::ostream* secondary_{nullptr};
    std::unique_ptr<std::ofstream> ownedSecondary_;
    std::optional<std::filesystem::path> secondaryFilePath_;
    LogLevel minLevel_;
    Sink sink_;
    mutable std::mutex mutex_;
};

}  // namespace optigrab
