#pragma once

#include "optigrab/domain/Session.hpp"
#include "optigrab/domain/Types.hpp"
#include "optigrab/ports/AudioEncoder.hpp"
#include "optigrab/ports/AudioExtractor.hpp"
#include "optigrab/ports/CoverArtApplier.hpp"
#include "optigrab/ports/CoverArtProvider.hpp"
#include "optigrab/ports/MetadataProvider.hpp"
#include "optigrab/ports/TocReader.hpp"

#include <functional>
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
    using LogFn = std::function<void(const std::string&)>;

    RipService(std::shared_ptr<TocReader> toc,
               std::shared_ptr<AudioExtractor> extractor,
               std::shared_ptr<AudioEncoder> encoder,
               std::shared_ptr<MetadataProvider> metadata,
               std::shared_ptr<CoverArtProvider> coverProvider = nullptr,
               std::shared_ptr<CoverArtApplier> coverApplier = nullptr);

    void loadDisc(Session& session, LogFn log = {});

    // Serial pipeline:
    //   1) optional cover download
    //   2) extract+encode all tracks
    //   3) if cover: sidecar once + embed each successful MP3
    std::vector<RipResult> ripTracks(Session& session,
                                     const std::vector<int>& trackNumbers,
                                     LogFn log = {});

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
