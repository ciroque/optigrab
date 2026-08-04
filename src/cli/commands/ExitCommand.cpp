#include "optigrab/cli/Command.hpp"

#include <memory>

namespace optigrab {
namespace {

class ExitCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        ctx.shouldExit = true;
        ctx.out << "Bye.\n";
    }
    [[nodiscard]] std::string name() const override { return "exit"; }
};

}  // namespace

std::unique_ptr<Command> makeExitCommand() { return std::make_unique<ExitCommand>(); }

}  // namespace optigrab
