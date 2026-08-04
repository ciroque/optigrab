#pragma once

#include "optigrab/domain/Session.hpp"
#include "optigrab/domain/Types.hpp"
#include "optigrab/ports/AudioEncoder.hpp"
#include "optigrab/ports/AudioExtractor.hpp"
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
               std::shared_ptr<MetadataProvider> metadata);

    // Refresh TOC (+ metadata) into the session for the selected drive.
    void loadDisc(Session& session, LogFn log = {});

    // Rip the given 1-based track numbers.
    std::vector<RipResult> ripTracks(Session& session,
                                     const std::vector<int>& trackNumbers,
                                     LogFn log = {});

private:
    Tags makeTags(const Session& session, const TrackInfo& track, int trackTotal) const;

    std::shared_ptr<TocReader> toc_;
    std::shared_ptr<AudioExtractor> extractor_;
    std::shared_ptr<AudioEncoder> encoder_;
    std::shared_ptr<MetadataProvider> metadata_;
};

}  // namespace optigrab
