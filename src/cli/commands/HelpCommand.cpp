#include "optigrab/cli/Command.hpp"

#include <memory>

namespace optigrab {
namespace {

class HelpCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>&) override {
        ctx.out <<
            R"(optigrab — optical disc grabber (diskpart-style VERB NOUN)

Commands:
  list drive              List optical drives
  select drive <n|path>   Select a drive (session focus)
  list track              List tracks on the selected disc
  detail drive            Show selected drive / session settings
  detail disc             Show disc summary
  rip track <spec>        Rip tracks (all | N | N-M | N,M,...)
  set out <dir>           Output directory
  set quality <preset>    V0 | V2 | 192 | 256 | 320
  set artist <name>       Album artist (tags + folder naming)
  set album <name>        Album name (tags + folder naming)
  set extractor <name>    ffmpeg | cdparanoia | libcdio
  set encoder <name>      ffmpeg
  help                    This help
  cls                     Clear screen
  exit                    Quit

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
