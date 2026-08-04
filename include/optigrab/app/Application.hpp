#pragma once

#include "optigrab/app/CompositionRoot.hpp"
#include "optigrab/cli/CommandHandler.hpp"
#include "optigrab/cli/Context.hpp"

#include <memory>
#include <string>

namespace optigrab {

class Application {
public:
    Application();
    void run();
    // Execute a single line (for tests / scripting).
    void executeLine(const std::string& line);
    [[nodiscard]] Context& context();

private:
    AppServices services_;
    std::unique_ptr<Context> ctx_;
    CommandHandler handler_;
};

}  // namespace optigrab
