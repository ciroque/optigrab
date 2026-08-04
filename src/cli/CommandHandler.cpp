#include "optigrab/cli/CommandHandler.hpp"

#include "optigrab/cli/Tokenizer.hpp"
#include "optigrab/domain/Errors.hpp"

#include <cctype>

namespace optigrab {

// Factory decls (implemented in command translation units).
std::unique_ptr<Command> makeListDriveCommand();
std::unique_ptr<Command> makeListTrackCommand();
std::unique_ptr<Command> makeSelectDriveCommand();
std::unique_ptr<Command> makeDetailDriveCommand();
std::unique_ptr<Command> makeDetailDiscCommand();
std::unique_ptr<Command> makeSetOutCommand();
std::unique_ptr<Command> makeSetQualityCommand();
std::unique_ptr<Command> makeSetArtistCommand();
std::unique_ptr<Command> makeSetAlbumCommand();
std::unique_ptr<Command> makeSetExtractorCommand();
std::unique_ptr<Command> makeSetEncoderCommand();
std::unique_ptr<Command> makeRipTrackCommand();
std::unique_ptr<Command> makeHelpCommand();
std::unique_ptr<Command> makeExitCommand();
std::unique_ptr<Command> makeClsCommand();

void CommandHandler::addCommand(std::unique_ptr<Command> cmd) {
    const auto n = cmd->name();
    if (n.find(' ') == std::string::npos) {
        unary_[n] = std::move(cmd);
    } else {
        commands_[n] = std::move(cmd);
    }
}

void CommandHandler::execute(Context& ctx, const std::string& line) {
    std::vector<std::string> tokens;
    try {
        tokens = tokenize(line);
    } catch (const OptigrabError& ex) {
        ctx.err << ex.what() << "\n";
        return;
    }

    if (tokens.empty()) {
        return;
    }

    try {
        if (tokens.size() == 1) {
            auto it = unary_.find(tokens[0]);
            if (it == unary_.end()) {
                // Allow "help" style; also try as unknown
                ctx.err << "Unknown command: " << tokens[0] << "\n";
                ctx.err << "Type 'help' for a list of commands.\n";
                return;
            }
            it->second->execute(ctx, tokens);
            return;
        }

        const std::string key = tokens[0] + " " + tokens[1];
        auto it = commands_.find(key);
        if (it == commands_.end()) {
            // Verb known but noun wrong?
            bool verbExists = false;
            for (const auto& [name, _] : commands_) {
                if (name.rfind(tokens[0] + " ", 0) == 0) {
                    verbExists = true;
                    break;
                }
            }
            if (verbExists) {
                ctx.err << "Unknown noun for verb '" << tokens[0] << "': " << tokens[1] << "\n";
            } else {
                ctx.err << "Unknown command: " << key << "\n";
            }
            ctx.err << "Type 'help' for a list of commands.\n";
            return;
        }
        it->second->execute(ctx, tokens);
    } catch (const OptigrabError& ex) {
        ctx.err << ex.what() << "\n";
    } catch (const std::exception& ex) {
        ctx.err << "Error: " << ex.what() << "\n";
    }
}

std::vector<std::string> CommandHandler::registeredCommands() const {
    std::vector<std::string> names;
    names.reserve(commands_.size() + unary_.size());
    for (const auto& [n, _] : unary_) {
        names.push_back(n);
    }
    for (const auto& [n, _] : commands_) {
        names.push_back(n);
    }
    return names;
}

CommandHandler makeDefaultCommandHandler() {
    CommandHandler h;
    h.addCommand(makeListDriveCommand());
    h.addCommand(makeListTrackCommand());
    h.addCommand(makeSelectDriveCommand());
    h.addCommand(makeDetailDriveCommand());
    h.addCommand(makeDetailDiscCommand());
    h.addCommand(makeSetOutCommand());
    h.addCommand(makeSetQualityCommand());
    h.addCommand(makeSetArtistCommand());
    h.addCommand(makeSetAlbumCommand());
    h.addCommand(makeSetExtractorCommand());
    h.addCommand(makeSetEncoderCommand());
    h.addCommand(makeRipTrackCommand());
    h.addCommand(makeHelpCommand());
    h.addCommand(makeExitCommand());
    h.addCommand(makeClsCommand());
    return h;
}

}  // namespace optigrab
