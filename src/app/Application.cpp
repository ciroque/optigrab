#include "optigrab/app/Application.hpp"

#include <iostream>

namespace optigrab {

Application::Application()
    : services_(makeDefaultServices()),
      ctx_(makeContext(services_)),
      handler_(makeDefaultCommandHandler()) {}

void Application::run() {
    ctx_->out << "optigrab " << "0.1.0" << " — type 'help', 'exit' to quit\n";
    ctx_->out << "OPTIGRAB> ";
    std::string line;
    while (!ctx_->shouldExit && std::getline(std::cin, line)) {
        if (!line.empty()) {
            handler_.execute(*ctx_, line);
        }
        if (ctx_->shouldExit) {
            break;
        }
        ctx_->out << "OPTIGRAB> ";
    }
}

void Application::executeLine(const std::string& line) { handler_.execute(*ctx_, line); }

Context& Application::context() { return *ctx_; }

}  // namespace optigrab
