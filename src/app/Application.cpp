#include "optigrab/app/Application.hpp"

#include "optigrab/cli/LineReader.hpp"

#include <iostream>

namespace optigrab {

Application::Application()
    : services_(makeDefaultServices()),
      ctx_(makeContext(services_)),
      handler_(makeDefaultCommandHandler()) {}

void Application::run() {
    ctx_->out << "optigrab " << "0.1.0" << " — type 'help', 'exit' to quit\n";

    LineReader reader(history_, ctx_->out, std::cin);
    constexpr const char* kPrompt = "OPTIGRAB> ";

    while (!ctx_->shouldExit) {
        const auto line = reader.readLine(kPrompt);
        if (!line) {
            break;  // EOF
        }
        if (line->empty()) {
            continue;
        }
        history_.add(*line);
        handler_.execute(*ctx_, *line);
    }
}

void Application::executeLine(const std::string& line) {
    if (!line.empty()) {
        history_.add(line);
    }
    handler_.execute(*ctx_, line);
}

Context& Application::context() { return *ctx_; }

History& Application::history() { return history_; }

}  // namespace optigrab
