#include "optigrab/cli/Command.hpp"

#include "optigrab/platform/Platform.hpp"

#include <memory>

namespace optigrab {
namespace {

class HelpCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
#ifndef OPTIGRAB_VERSION_STRING
#define OPTIGRAB_VERSION_STRING "dev"
#endif
        ctx.out << "optigrab " << OPTIGRAB_VERSION_STRING << " (" << platformName()
                << ") — optical disc grabber (diskpart-style VERB NOUN)\n\n";
        ctx.out <<
            R"(Commands:
  list drive              List optical drives
  select drive <n|path>   Select a drive (session focus)
  eject drive [n|path]    Eject tray (selected drive if omitted)
  list track              List tracks on the selected disc
  detail drive            Show selected drive / session settings
  detail disc             Show disc summary
  rip track <spec>        Rip tracks (all | N | N-M | N,M,...)
  set out <dir>           Output directory
  set quality <preset>    V0 | V2 | 192 | 256 | 320
  set artist <name>       Album artist (tags + folder naming)
  set album <name>        Album name (tags + folder naming)
  set cover <path>|none   Local cover image (preferred over network)
  set coverart on|off     Enable/disable cover download+embed
  set covermissing <p>    ask|continue|abort if no cover (default: ask)
  set folderlayout <l>    nested|joined|album (default: nested)
                            nested  out/Artist/Album/track.mp3
                            joined  out/Artist - Album/track.mp3
                            album   out/Album/track.mp3
  set loglevel <level>    trace|debug|info|warn|error|fatal|off
  set logfile <path>|none Tee logger to file (append); still prints to stderr
)";
#ifdef _WIN32
        ctx.out << "  set extractor <name>    ffmpeg\n";
#else
        ctx.out << "  set extractor <name>    ffmpeg | cdparanoia | libcdio\n";
#endif
        ctx.out <<
            R"(  set encoder <name>      ffmpeg
  help                    This help
  cls                     Clear screen
  exit                    Quit

Input:
  Up / Down               Recall previous commands

Examples:
  list drive
  select drive 0
  set artist "The Band"
  set album "Live"
  set out ~/Music
  list track
  rip track all
)";
    }
    [[nodiscard]] std::string name() const override { return "help"; }
};

}  // namespace

std::unique_ptr<Command> makeHelpCommand() { return std::make_unique<HelpCommand>(); }

}  // namespace optigrab
