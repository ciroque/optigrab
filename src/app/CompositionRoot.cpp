#include "optigrab/app/CompositionRoot.hpp"

#include "optigrab/adapters/cdparanoia/CdparanoiaExtractor.hpp"
#include "optigrab/adapters/ffmpeg/FfmpegEncoder.hpp"
#include "optigrab/adapters/ffmpeg/FfmpegExtractor.hpp"
#include "optigrab/adapters/libcdio/LibcdioParanoiaExtractor.hpp"
#include "optigrab/adapters/libcdio/LibcdioTocReader.hpp"
#include "optigrab/adapters/linux/LinuxDriveEnumerator.hpp"
#include "optigrab/adapters/manual/ManualMetadataProvider.hpp"
#include "optigrab/domain/Errors.hpp"

namespace optigrab {

std::shared_ptr<AudioExtractor> AppServices::makeExtractor(ExtractorKind kind) {
    switch (kind) {
    case ExtractorKind::Ffmpeg:
        return std::make_shared<FfmpegExtractor>();
    case ExtractorKind::Cdparanoia:
        return std::make_shared<CdparanoiaExtractor>();
    case ExtractorKind::LibcdioParanoia:
        return std::make_shared<LibcdioParanoiaExtractor>();
    }
    throw OptigrabError("Unknown extractor kind");
}

std::shared_ptr<AudioEncoder> AppServices::makeEncoder(EncoderKind kind) {
    switch (kind) {
    case EncoderKind::Ffmpeg:
        return std::make_shared<FfmpegEncoder>();
    }
    throw OptigrabError("Unknown encoder kind");
}

std::shared_ptr<RipService> AppServices::makeRipper(ExtractorKind extractor, EncoderKind encoder) {
    return std::make_shared<RipService>(toc, makeExtractor(extractor), makeEncoder(encoder),
                                        metadata);
}

AppServices makeDefaultServices() {
    AppServices s;
    s.drives = std::make_shared<LinuxDriveEnumerator>();
    s.toc = std::make_shared<LibcdioTocReader>();
    s.metadata = std::make_shared<ManualMetadataProvider>();
    return s;
}

std::unique_ptr<Context> makeContext(AppServices& services, std::ostream& out, std::ostream& err) {
    auto ripper = services.makeRipper(ExtractorKind::Cdparanoia, EncoderKind::Ffmpeg);
    auto rebuild = [&services](ExtractorKind ex, EncoderKind en) {
        return services.makeRipper(ex, en);
    };
    return std::make_unique<Context>(services.drives, std::move(ripper), std::move(rebuild), out,
                                     err);
}

}  // namespace optigrab
