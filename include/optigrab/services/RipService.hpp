#pragma once

#include "optigrab/domain/Session.hpp"
#include "optigrab/domain/Types.hpp"
#include "optigrab/log/Logger.hpp"
#include "optigrab/ports/AudioEncoder.hpp"
#include "optigrab/ports/AudioExtractor.hpp"
#include "optigrab/ports/CoverArtApplier.hpp"
#include "optigrab/ports/CoverArtProvider.hpp"
#include "optigrab/ports/MetadataProvider.hpp"
#include "optigrab/ports/TocReader.hpp"

#include <memory>
#include <vector>

namespace optigrab {

struct RipResult {
    int trackNumber{0};
    std::filesystem::path outputPath;
    bool success{false};
    std::string message;
};

class RipService {
public:
    RipService(std::shared_ptr<TocReader> toc,
               std::shared_ptr<AudioExtractor> extractor,
               std::shared_ptr<AudioEncoder> encoder,
               std::shared_ptr<MetadataProvider> metadata,
               std::shared_ptr<CoverArtProvider> coverProvider = nullptr,
               std::shared_ptr<CoverArtApplier> coverApplier = nullptr);

    void loadDisc(Session& session, Logger* log = nullptr);

    // Serial: cover fetch → extract/encode all → sidecar + embed if cover exists.
    std::vector<RipResult> ripTracks(Session& session,
                                     const std::vector<int>& trackNumbers,
                                     Logger* log = nullptr);

private:
    Tags makeTags(const Session& session, const TrackInfo& track, int trackTotal) const;

    std::shared_ptr<TocReader> toc_;
    std::shared_ptr<AudioExtractor> extractor_;
    std::shared_ptr<AudioEncoder> encoder_;
    std::shared_ptr<MetadataProvider> metadata_;
    std::shared_ptr<CoverArtProvider> coverProvider_;
    std::shared_ptr<CoverArtApplier> coverApplier_;
};

}  // namespace optigrab
