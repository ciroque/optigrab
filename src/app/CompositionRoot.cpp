#include "optigrab/app/CompositionRoot.hpp"

#include "optigrab/adapters/cover/CompositeCoverArtProvider.hpp"
#include "optigrab/adapters/cover/LocalCoverArtProvider.hpp"
#include "optigrab/adapters/cover/MusicBrainzCoverArtProvider.hpp"
#include "optigrab/adapters/ffmpeg/FfmpegCoverArtApplier.hpp"
#include "optigrab/adapters/ffmpeg/FfmpegEncoder.hpp"
#include "optigrab/adapters/ffmpeg/FfmpegExtractor.hpp"
#include "optigrab/adapters/manual/ManualMetadataProvider.hpp"
#include "optigrab/domain/Errors.hpp"
#include "optigrab/platform/Platform.hpp"

#ifdef _WIN32
#include "optigrab/adapters/windows/WindowsDriveEjector.hpp"
#include "optigrab/adapters/windows/WindowsDriveEnumerator.hpp"
#include "optigrab/adapters/windows/WindowsTocReader.hpp"
#else
#include "optigrab/adapters/cdparanoia/CdparanoiaExtractor.hpp"
#include "optigrab/adapters/libcdio/LibcdioParanoiaExtractor.hpp"
#include "optigrab/adapters/libcdio/LibcdioTocReader.hpp"
#include "optigrab/adapters/linux/LinuxDriveEjector.hpp"
#include "optigrab/adapters/linux/LinuxDriveEnumerator.hpp"
#endif

namespace optigrab {

std::shared_ptr<AudioExtractor> AppServices::makeExtractor(ExtractorKind kind) {
    switch (kind) {
    case ExtractorKind::Ffmpeg:
        return std::make_shared<FfmpegExtractor>();
    case ExtractorKind::Cdparanoia:
#ifdef _WIN32
        throw OptigrabError("cdparanoia is not available on Windows; use ffmpeg or libcdio");
#else
        return std::make_shared<CdparanoiaExtractor>();
#endif
    case ExtractorKind::LibcdioParanoia:
#ifdef _WIN32
        throw OptigrabError(
            "libcdio_paranoia is not built into this Windows binary; use: set extractor ffmpeg");
#else
        return std::make_shared<LibcdioParanoiaExtractor>();
#endif
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
                                        metadata, cover, coverApplier);
}

AppServices makeDefaultServices() {
    AppServices s;
#ifdef _WIN32
    s.drives = std::make_shared<WindowsDriveEnumerator>();
    s.ejector = std::make_shared<WindowsDriveEjector>();
    s.toc = std::make_shared<WindowsTocReader>();
#else
    s.drives = std::make_shared<LinuxDriveEnumerator>();
    s.ejector = std::make_shared<LinuxDriveEjector>();
    s.toc = std::make_shared<LibcdioTocReader>();
#endif
    s.metadata = std::make_shared<ManualMetadataProvider>();

    std::vector<std::shared_ptr<CoverArtProvider>> coverProviders;
    coverProviders.push_back(std::make_shared<LocalCoverArtProvider>());
    coverProviders.push_back(std::make_shared<MusicBrainzCoverArtProvider>());
    s.cover = std::make_shared<CompositeCoverArtProvider>(std::move(coverProviders));
    s.coverApplier = std::make_shared<FfmpegCoverArtApplier>();
    return s;
}

std::unique_ptr<Context> makeContext(AppServices& services, std::ostream& out, std::ostream& err) {
    const auto extractor = defaultExtractor();
    auto ripper = services.makeRipper(extractor, EncoderKind::Ffmpeg);
    auto rebuild = [&services](ExtractorKind ex, EncoderKind en) {
        return services.makeRipper(ex, en);
    };
    auto ctx = std::make_unique<Context>(services.drives, std::move(ripper), std::move(rebuild), out,
                                         err, services.ejector);
    ctx->session.setExtractor(extractor);
    return ctx;
}

}  // namespace optigrab
