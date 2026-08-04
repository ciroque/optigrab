#include "optigrab/cli/Command.hpp"

#include <memory>

namespace optigrab {
namespace {

class ClsCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        // ANSI clear; harmless on dumb terminals if ignored partially.
        ctx.out << "\033[2J\033[H";
    }
    [[nodiscard]] std::string name() const override { return "cls"; }
};

}  // namespace

std::unique_ptr<Command> makeClsCommand() { return std::make_unique<ClsCommand>(); }

}  // namespace optigrab
