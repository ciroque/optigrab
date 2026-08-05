#include "optigrab/cli/Context.hpp"

#include "optigrab/domain/Errors.hpp"

namespace optigrab {

void Context::setLogFile(const std::string& path) {
    auto file = std::make_unique<std::ofstream>(path, std::ios::out | std::ios::app);
    if (!file || !file->is_open()) {
        throw OptigrabError("Failed to open log file: " + path);
    }
    logFileStream_ = std::move(file);
    logFilePath_ = path;
    log.setSecondaryStream(logFileStream_.get());
    log.info("logging also to " + path);
}

void Context::clearLogFile() {
    log.setSecondaryStream(nullptr);
    logFileStream_.reset();
    if (logFilePath_) {
        const auto prev = *logFilePath_;
        logFilePath_.reset();
        log.info("stopped logging to " + prev);
    } else {
        logFilePath_.reset();
    }
}

}  // namespace optigrab
