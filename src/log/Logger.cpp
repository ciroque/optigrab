#include "optigrab/log/Logger.hpp"

#include <chrono>
#include <iomanip>
#include <sstream>
#include <stdexcept>

namespace optigrab {
namespace {

std::string timestampNow() {
    using clock = std::chrono::system_clock;
    const auto now = clock::now();
    const auto t = clock::to_time_t(now);
    std::tm tm{};
#if defined(_WIN32)
    localtime_s(&tm, &t);
#else
    localtime_r(&t, &tm);
#endif
    std::ostringstream oss;
    oss << std::put_time(&tm, "%Y-%m-%d %H:%M:%S");
    return oss.str();
}

std::string formatLine(LogLevel level, std::string_view message) {
    std::ostringstream oss;
    oss << timestampNow() << " [" << toString(level) << "] " << message << '\n';
    return oss.str();
}

}  // namespace

Logger::Logger(std::ostream& stream, LogLevel minLevel)
    : stream_(&stream), minLevel_(minLevel) {}

void Logger::setLevel(LogLevel level) {
    std::lock_guard lock(mutex_);
    minLevel_ = level;
}

LogLevel Logger::level() const {
    std::lock_guard lock(mutex_);
    return minLevel_;
}

void Logger::setStream(std::ostream& stream) {
    std::lock_guard lock(mutex_);
    stream_ = &stream;
}

void Logger::clearOwnedSecondaryUnlocked() {
    secondary_ = nullptr;
    ownedSecondary_.reset();
    secondaryFilePath_.reset();
}

void Logger::setSecondaryStream(std::ostream* stream) {
    std::lock_guard lock(mutex_);
    clearOwnedSecondaryUnlocked();
    secondary_ = stream;
}

std::ostream* Logger::secondaryStream() const {
    std::lock_guard lock(mutex_);
    return secondary_;
}

bool Logger::tryOpenSecondaryFile(const std::filesystem::path& path, std::string& errorOut) {
    std::lock_guard lock(mutex_);
    clearOwnedSecondaryUnlocked();

    std::error_code ec;
    const auto parent = path.parent_path();
    if (!parent.empty()) {
        std::filesystem::create_directories(parent, ec);
        if (ec) {
            errorOut = "Failed to create log directory " + parent.string() + ": " + ec.message();
            return false;
        }
    }

    auto file = std::make_unique<std::ofstream>(path, std::ios::out | std::ios::app);
    if (!file || !file->is_open()) {
        errorOut = "Failed to open log file: " + path.string();
        return false;
    }
    ownedSecondary_ = std::move(file);
    secondary_ = ownedSecondary_.get();
    secondaryFilePath_ = path;
    return true;
}

void Logger::setSecondaryFile(const std::filesystem::path& path) {
    std::string err;
    if (!tryOpenSecondaryFile(path, err)) {
        throw std::runtime_error(err);
    }
}

void Logger::clearSecondaryFile() {
    std::lock_guard lock(mutex_);
    clearOwnedSecondaryUnlocked();
}

const std::optional<std::filesystem::path>& Logger::secondaryFilePath() const {
    std::lock_guard lock(mutex_);
    return secondaryFilePath_;
}

void Logger::setSink(Sink sink) {
    std::lock_guard lock(mutex_);
    sink_ = std::move(sink);
}

void Logger::clearSink() {
    std::lock_guard lock(mutex_);
    sink_ = nullptr;
}

bool Logger::enabled(LogLevel level) const {
    std::lock_guard lock(mutex_);
    return static_cast<int>(level) >= static_cast<int>(minLevel_) &&
           minLevel_ != LogLevel::Off && level != LogLevel::Off;
}

void Logger::emit(LogLevel level, std::string_view message) {
    if (minLevel_ == LogLevel::Off || level == LogLevel::Off) {
        return;
    }
    if (static_cast<int>(level) < static_cast<int>(minLevel_)) {
        return;
    }
    if (sink_) {
        sink_(level, message);
        return;
    }
    const std::string line = formatLine(level, message);
    if (stream_) {
        (*stream_) << line;
        stream_->flush();
    }
    if (secondary_) {
        (*secondary_) << line;
        secondary_->flush();
    }
}

void Logger::log(LogLevel level, std::string_view message) {
    std::lock_guard lock(mutex_);
    emit(level, message);
}

void Logger::trace(std::string_view message) { log(LogLevel::Trace, message); }
void Logger::debug(std::string_view message) { log(LogLevel::Debug, message); }
void Logger::info(std::string_view message) { log(LogLevel::Info, message); }
void Logger::warn(std::string_view message) { log(LogLevel::Warn, message); }
void Logger::error(std::string_view message) { log(LogLevel::Error, message); }
void Logger::fatal(std::string_view message) { log(LogLevel::Fatal, message); }

std::function<void(const std::string&)> Logger::callback(LogLevel level) {
    return [this, level](const std::string& message) { log(level, message); };
}

}  // namespace optigrab
