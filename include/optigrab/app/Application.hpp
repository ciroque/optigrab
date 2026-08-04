#pragma once

#include "optigrab/app/CompositionRoot.hpp"
#include "optigrab/cli/Args.hpp"
#include "optigrab/cli/CommandHandler.hpp"
#include "optigrab/cli/Context.hpp"
#include "optigrab/cli/History.hpp"

#include <memory>
#include <string>
#include <vector>

namespace optigrab {

class Application {
public:
    Application();

    // Interactive REPL (auto-selects sole drive).
    void runInteractive();

    // Full entry: no args → interactive; otherwise apply flags and run one command.
    // Returns process exit code.
    int run(int argc, char** argv);

    // Execute a single line (for tests / scripting).
    void executeLine(const std::string& line);
    [[nodiscard]] Context& context();
    [[nodiscard]] History& history();

private:
    void applyLaunchArgs(const LaunchArgs& args);
    int runOneShot(const LaunchArgs& args);

    AppServices services_;
    std::unique_ptr<Context> ctx_;
    CommandHandler handler_;
    History history_;
};

}  // namespace optigrab
