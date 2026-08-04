#pragma once

#include "optigrab/app/CompositionRoot.hpp"
#include "optigrab/cli/CommandHandler.hpp"
#include "optigrab/cli/Context.hpp"
#include "optigrab/cli/History.hpp"

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
    [[nodiscard]] History& history();

private:
    AppServices services_;
    std::unique_ptr<Context> ctx_;
    CommandHandler handler_;
    History history_;
};

}  // namespace optigrab
