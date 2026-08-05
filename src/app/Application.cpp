#include "optigrab/app/Application.hpp"

#include "optigrab/cli/DriveSelection.hpp"
#include "optigrab/cli/LineReader.hpp"
#include "optigrab/domain/Errors.hpp"

#include <iostream>
#include <sstream>

namespace optigrab {

Application::Application()
    : services_(makeDefaultServices()),
      ctx_(makeContext(services_)),
      handler_(makeDefaultCommandHandler()) {}

void Application::applyLaunchArgs(const LaunchArgs& args) {
    if (args.outDir) {
        ctx_->session.setOutputDirectory(*args.outDir);
    }
    if (args.artist) {
        ctx_->session.setArtist(*args.artist);
    }
    if (args.album) {
        ctx_->session.setAlbum(*args.album);
    }
    if (args.coverPath) {
        ctx_->session.setCoverPath(*args.coverPath);
    }
    if (args.fetchCoverArt) {
        ctx_->session.setFetchCoverArt(*args.fetchCoverArt);
    }
    if (args.coverMissing) {
        ctx_->session.setCoverMissingPolicy(*args.coverMissing);
    } else if (!args.interactive) {
        // One-shot default: ask (session already defaults to ask; restate for clarity).
        ctx_->session.setCoverMissingPolicy(CoverMissingPolicy::Ask);
    }
    if (args.logLevel) {
        ctx_->log.setLevel(*args.logLevel);
        ctx_->log.debug(std::string("log level set to ") + toString(*args.logLevel));
    }
    if (args.quality) {
        ctx_->session.setQuality(*args.quality);
    }
    if (args.extractor) {
        ctx_->session.setExtractor(*args.extractor);
        if (ctx_->rebuildRipper) {
            ctx_->ripper = ctx_->rebuildRipper(ctx_->session.extractor(), ctx_->session.encoder());
        }
    }
    if (args.encoder) {
        ctx_->session.setEncoder(*args.encoder);
        if (ctx_->rebuildRipper) {
            ctx_->ripper = ctx_->rebuildRipper(ctx_->session.extractor(), ctx_->session.encoder());
        }
    }
    if (args.drive) {
        // Reuse select drive command path.
        handler_.execute(*ctx_, "select drive " + *args.drive);
        if (ctx_->exitCode != 0) {
            return;
        }
    }
}

void Application::runInteractive() {
#ifndef OPTIGRAB_VERSION_STRING
#define OPTIGRAB_VERSION_STRING "dev"
#endif
    ctx_->out << "optigrab " << OPTIGRAB_VERSION_STRING << " — type 'help', 'exit' to quit\n";
    tryAutoSelectSingleDrive(*ctx_, true);

    LineReader reader(history_, ctx_->out, std::cin);
    constexpr const char* kPrompt = "OPTIGRAB> ";

    while (!ctx_->shouldExit) {
        const auto line = reader.readLine(kPrompt);
        if (!line) {
            break;
        }
        if (line->empty()) {
            continue;
        }
        history_.add(*line);
        handler_.execute(*ctx_, *line);
    }
}

int Application::runOneShot(const LaunchArgs& args) {
    if (!args.drive) {
        tryAutoSelectSingleDrive(*ctx_, true);
    }

    std::ostringstream cmd;
    for (std::size_t i = 0; i < args.command.size(); ++i) {
        if (i) {
            cmd << ' ';
        }
        // Re-quote tokens with spaces for the tokenizer.
        if (args.command[i].find(' ') != std::string::npos) {
            cmd << '"' << args.command[i] << '"';
        } else {
            cmd << args.command[i];
        }
    }

    handler_.execute(*ctx_, cmd.str());
    return ctx_->exitCode;
}

int Application::run(int argc, char** argv) {
#ifndef OPTIGRAB_VERSION_STRING
#define OPTIGRAB_VERSION_STRING "dev"
#endif
    std::vector<std::string> argsVec;
    argsVec.reserve(static_cast<std::size_t>(argc > 0 ? argc - 1 : 0));
    for (int i = 1; i < argc; ++i) {
        argsVec.emplace_back(argv[i]);
    }

    LaunchArgs launch;
    try {
        launch = parseLaunchArgs(argsVec);
    } catch (const OptigrabError& ex) {
        ctx_->log.error(ex.what());
        ctx_->log.info("try: optigrab --help");
        return 1;
    }

    if (launch.showVersion) {
        ctx_->out << "optigrab " << OPTIGRAB_VERSION_STRING << "\n";
        return 0;
    }
    if (launch.showHelp) {
        ctx_->out << usageText();
        return 0;
    }

    applyLaunchArgs(launch);
    if (ctx_->exitCode != 0) {
        return ctx_->exitCode;
    }

    if (launch.interactive) {
        runInteractive();
        return ctx_->exitCode;
    }

    return runOneShot(launch);
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
