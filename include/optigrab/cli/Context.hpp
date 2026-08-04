#pragma once

#include "optigrab/domain/Session.hpp"
#include "optigrab/ports/DriveEnumerator.hpp"
#include "optigrab/services/RipService.hpp"

#include <functional>
#include <iostream>
#include <memory>

namespace optigrab {

struct Context {
    Session session;
    std::shared_ptr<DriveEnumerator> drives;
    std::shared_ptr<RipService> ripper;
    // Factory to rebuild RipService when extractor/encoder selection changes.
    std::function<std::shared_ptr<RipService>(ExtractorKind, EncoderKind)> rebuildRipper;
    std::ostream& out;
    std::ostream& err;
    bool shouldExit{false};

    Context(std::shared_ptr<DriveEnumerator> driveEnum,
            std::shared_ptr<RipService> ripService,
            std::function<std::shared_ptr<RipService>(ExtractorKind, EncoderKind)> rebuild,
            std::ostream& output = std::cout,
            std::ostream& error = std::cerr)
        : drives(std::move(driveEnum)),
          ripper(std::move(ripService)),
          rebuildRipper(std::move(rebuild)),
          out(output),
          err(error) {}
};

}  // namespace optigrab
