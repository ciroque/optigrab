#pragma once

#include "optigrab/cli/Command.hpp"
#include "optigrab/cli/Context.hpp"

#include <map>
#include <memory>
#include <string>
#include <vector>

namespace optigrab {

class CommandHandler {
public:
    void addCommand(std::unique_ptr<Command> cmd);
    void execute(Context& ctx, const std::string& line);
    [[nodiscard]] std::vector<std::string> registeredCommands() const;

private:
    std::map<std::string, std::unique_ptr<Command>> commands_;
    std::map<std::string, std::unique_ptr<Command>> unary_;  // help, exit, cls
};

// Builds the default VERB NOUN command set.
[[nodiscard]] CommandHandler makeDefaultCommandHandler();

}  // namespace optigrab
