#pragma once

#include "optigrab/cli/Context.hpp"

#include <string>
#include <vector>

namespace optigrab {

class Command {
public:
    virtual ~Command() = default;
    // tokens[0]=verb, tokens[1]=noun (if any), rest=args
    virtual void execute(Context& ctx, const std::vector<std::string>& tokens) = 0;
    [[nodiscard]] virtual std::string name() const = 0;  // e.g. "list drive"
};

}  // namespace optigrab
