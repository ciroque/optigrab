#pragma once

#include "optigrab/cli/Context.hpp"
#include "optigrab/domain/Types.hpp"
#include "optigrab/ports/AudioEncoder.hpp"
#include "optigrab/ports/AudioExtractor.hpp"
#include "optigrab/ports/CoverArtApplier.hpp"
#include "optigrab/ports/CoverArtProvider.hpp"
#include "optigrab/ports/DriveEnumerator.hpp"
#include "optigrab/ports/MetadataProvider.hpp"
#include "optigrab/ports/TocReader.hpp"
#include "optigrab/services/RipService.hpp"

#include <memory>

namespace optigrab {

struct AppServices {
    std::shared_ptr<DriveEnumerator> drives;
    std::shared_ptr<TocReader> toc;
    std::shared_ptr<MetadataProvider> metadata;
    std::shared_ptr<CoverArtProvider> cover;
    std::shared_ptr<CoverArtApplier> coverApplier;
    std::shared_ptr<AudioExtractor> makeExtractor(ExtractorKind kind);
    std::shared_ptr<AudioEncoder> makeEncoder(EncoderKind kind);
    std::shared_ptr<RipService> makeRipper(ExtractorKind extractor, EncoderKind encoder);
};

// Platform defaults: Linux enumerator + libcdio TOC + manual/null metadata.
[[nodiscard]] AppServices makeDefaultServices();

[[nodiscard]] std::unique_ptr<Context> makeContext(AppServices& services,
                                                   std::ostream& out = std::cout,
                                                   std::ostream& err = std::cerr);

}  // namespace optigrab
