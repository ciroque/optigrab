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
std::unique_ptr<Command> makeSetCoverCommand();
std::unique_ptr<Command> makeSetCoverArtCommand();
std::unique_ptr<Command> makeSetLogLevelCommand();
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

bool CommandHandler::execute(Context& ctx, const std::string& line) {
    auto fail = [&](const std::string& msg) {
        ctx.log.error(msg);
        if (ctx.exitCode == 0) {
            ctx.exitCode = 1;
        }
        return false;
    };

    std::vector<std::string> tokens;
    try {
        tokens = tokenize(line);
    } catch (const OptigrabError& ex) {
        return fail(ex.what());
    }

    if (tokens.empty()) {
        return true;
    }

    try {
        if (tokens.size() == 1) {
            auto it = unary_.find(tokens[0]);
            if (it == unary_.end()) {
                ctx.log.error("unknown command: " + tokens[0]);
                ctx.log.info("type 'help' for a list of commands");
                if (ctx.exitCode == 0) {
                    ctx.exitCode = 1;
                }
                return false;
            }
            it->second->execute(ctx, tokens);
            return ctx.exitCode == 0;
        }

        const std::string key = tokens[0] + " " + tokens[1];
        auto it = commands_.find(key);
        if (it == commands_.end()) {
            bool verbExists = false;
            for (const auto& [name, _] : commands_) {
                if (name.rfind(tokens[0] + " ", 0) == 0) {
                    verbExists = true;
                    break;
                }
            }
            if (verbExists) {
                ctx.log.error("unknown noun for verb '" + tokens[0] + "': " + tokens[1]);
            } else {
                ctx.log.error("unknown command: " + key);
            }
            ctx.log.info("type 'help' for a list of commands");
            if (ctx.exitCode == 0) {
                ctx.exitCode = 1;
            }
            return false;
        }
        it->second->execute(ctx, tokens);
        return ctx.exitCode == 0;
    } catch (const OptigrabError& ex) {
        return fail(ex.what());
    } catch (const std::exception& ex) {
        return fail(std::string("Error: ") + ex.what());
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
    h.addCommand(makeSetCoverCommand());
    h.addCommand(makeSetCoverArtCommand());
    h.addCommand(makeSetLogLevelCommand());
    h.addCommand(makeRipTrackCommand());
    h.addCommand(makeHelpCommand());
    h.addCommand(makeExitCommand());
    h.addCommand(makeClsCommand());
    return h;
}

}  // namespace optigrab
