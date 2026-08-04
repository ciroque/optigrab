#include "optigrab/adapters/cdparanoia/CdparanoiaExtractor.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/util/Process.hpp"

namespace optigrab {

CdparanoiaExtractor::CdparanoiaExtractor(std::string binary) : binary_(std::move(binary)) {}

void CdparanoiaExtractor::extractTrack(const std::string& devicePath,
                                       const TrackInfo& track,
                                       const std::filesystem::path& outputWav,
                                       ProgressFn progress) {
    if (!track.audio) {
        throw ExtractError("Track is not audio");
    }
    if (progress) {
        progress("cdparanoia track " + std::to_string(track.number));
    }

    // cdparanoia [options] span [outfile]
    // -d device, -w force wav
    const std::vector<std::string> args = {
        binary_,
        "-d",
        devicePath,
        "-w",
        std::to_string(track.number),
        outputWav.string(),
    };

    try {
        runProcessOrThrow(args, "cdparanoia");
    } catch (const OptigrabError& ex) {
        throw ExtractError(ex.what());
    }
}

}  // namespace optigrab
