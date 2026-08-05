#pragma once

#include "optigrab/domain/Session.hpp"
#include "optigrab/log/Logger.hpp"
#include "optigrab/ports/DriveEjector.hpp"
#include "optigrab/ports/DriveEnumerator.hpp"
#include "optigrab/services/RipService.hpp"

#include <fstream>
#include <functional>
#include <iostream>
#include <memory>
#include <optional>
#include <string>

namespace optigrab {

struct Context {
    Session session;
    std::shared_ptr<DriveEnumerator> drives;
    std::shared_ptr<DriveEjector> ejector;
    std::shared_ptr<RipService> ripper;
    std::function<std::shared_ptr<RipService>(ExtractorKind, EncoderKind)> rebuildRipper;
    std::ostream& out;   // user-facing tables / command results (not teed to log file)
    std::ostream& err;   // primary logger stream (usually stderr)
    Logger log;
    bool shouldExit{false};
    int exitCode{0};

    Context(std::shared_ptr<DriveEnumerator> driveEnum,
            std::shared_ptr<RipService> ripService,
            std::function<std::shared_ptr<RipService>(ExtractorKind, EncoderKind)> rebuild,
            std::ostream& output = std::cout,
            std::ostream& error = std::cerr,
            std::shared_ptr<DriveEjector> driveEjector = nullptr)
        : drives(std::move(driveEnum)),
          ejector(std::move(driveEjector)),
          ripper(std::move(ripService)),
          rebuildRipper(std::move(rebuild)),
          out(output),
          err(error),
          log(error, LogLevel::Info) {}

    // Tee Logger lines to path (append) as well as stderr. Throws OptigrabError on open failure.
    void setLogFile(const std::string& path);
    void clearLogFile();
    [[nodiscard]] const std::optional<std::string>& logFilePath() const { return logFilePath_; }

private:
    std::unique_ptr<std::ofstream> logFileStream_;
    std::optional<std::string> logFilePath_;
};

}  // namespace optigrab
