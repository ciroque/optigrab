#include "optigrab/cli/Command.hpp"

#include "optigrab/domain/Errors.hpp"

#include <memory>
#include <sstream>

namespace optigrab {
namespace {

std::string joinFrom(const std::vector<std::string>& tokens, std::size_t start) {
    std::ostringstream oss;
    for (std::size_t i = start; i < tokens.size(); ++i) {
        if (i > start) {
            oss << ' ';
        }
        oss << tokens[i];
    }
    return oss.str();
}

QualityPreset parseQuality(const std::string& s) {
    if (s == "V0" || s == "v0") return QualityPreset::V0;
    if (s == "V2" || s == "v2") return QualityPreset::V2;
    if (s == "192") return QualityPreset::Cbr192;
    if (s == "256") return QualityPreset::Cbr256;
    if (s == "320") return QualityPreset::Cbr320;
    throw ParseError("Unknown quality: " + s + " (V0, V2, 192, 256, 320)");
}

ExtractorKind parseExtractor(const std::string& s) {
    if (s == "ffmpeg") return ExtractorKind::Ffmpeg;
    if (s == "cdparanoia") {
#ifdef _WIN32
        throw ParseError("cdparanoia is not available on Windows (use ffmpeg)");
#else
        return ExtractorKind::Cdparanoia;
#endif
    }
    if (s == "libcdio" || s == "libcdio_paranoia" || s == "paranoia") {
#ifdef _WIN32
        throw ParseError("libcdio extractor is not available in this Windows build (use ffmpeg)");
#else
        return ExtractorKind::LibcdioParanoia;
#endif
    }
#ifdef _WIN32
    throw ParseError("Unknown extractor: " + s + " (ffmpeg)");
#else
    throw ParseError("Unknown extractor: " + s + " (ffmpeg, cdparanoia, libcdio)");
#endif
}

EncoderKind parseEncoder(const std::string& s) {
    if (s == "ffmpeg") return EncoderKind::Ffmpeg;
    throw ParseError("Unknown encoder: " + s + " (ffmpeg)");
}

class SetOutCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (tokens.size() < 3) {
            throw ParseError("Usage: set out <directory>");
        }
        ctx.session.setOutputDirectory(joinFrom(tokens, 2));
        ctx.out << "Output directory: " << ctx.session.outputDirectory() << "\n";
    }
    [[nodiscard]] std::string name() const override { return "set out"; }
};

class SetQualityCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (tokens.size() < 3) {
            throw ParseError("Usage: set quality <V0|V2|192|256|320>");
        }
        ctx.session.setQuality(parseQuality(tokens[2]));
        ctx.out << "Quality: " << toString(ctx.session.quality()) << "\n";
    }
    [[nodiscard]] std::string name() const override { return "set quality"; }
};

class SetArtistCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (tokens.size() < 3) {
            throw ParseError("Usage: set artist <name>");
        }
        ctx.session.setArtist(joinFrom(tokens, 2));
        ctx.out << "Artist: " << *ctx.session.artist() << "\n";
    }
    [[nodiscard]] std::string name() const override { return "set artist"; }
};

class SetAlbumCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (tokens.size() < 3) {
            throw ParseError("Usage: set album <name>");
        }
        ctx.session.setAlbum(joinFrom(tokens, 2));
        ctx.out << "Album: " << *ctx.session.album() << "\n";
    }
    [[nodiscard]] std::string name() const override { return "set album"; }
};

class SetExtractorCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (tokens.size() < 3) {
            throw ParseError("Usage: set extractor <ffmpeg|cdparanoia|libcdio>");
        }
        ctx.session.setExtractor(parseExtractor(tokens[2]));
        if (ctx.rebuildRipper) {
            ctx.ripper = ctx.rebuildRipper(ctx.session.extractor(), ctx.session.encoder());
        }
        ctx.out << "Extractor: " << toString(ctx.session.extractor()) << "\n";
    }
    [[nodiscard]] std::string name() const override { return "set extractor"; }
};

class SetEncoderCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (tokens.size() < 3) {
            throw ParseError("Usage: set encoder <ffmpeg>");
        }
        ctx.session.setEncoder(parseEncoder(tokens[2]));
        if (ctx.rebuildRipper) {
            ctx.ripper = ctx.rebuildRipper(ctx.session.extractor(), ctx.session.encoder());
        }
        ctx.out << "Encoder: " << toString(ctx.session.encoder()) << "\n";
    }
    [[nodiscard]] std::string name() const override { return "set encoder"; }
};

class SetCoverCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (tokens.size() < 3) {
            throw ParseError("Usage: set cover <image-path>|none");
        }
        const auto path = joinFrom(tokens, 2);
        if (path == "none" || path == "off" || path == "clear") {
            ctx.session.clearCoverPath();
            ctx.out << "Local cover path cleared.\n";
            return;
        }
        ctx.session.setCoverPath(path);
        ctx.out << "Cover: " << path << "\n";
    }
    [[nodiscard]] std::string name() const override { return "set cover"; }
};

class SetCoverArtCommand : public Command {
public:
    void execute(Context& ctx, const std::vector<std::string>& tokens) override {
        if (tokens.size() < 3) {
            throw ParseError("Usage: set coverart <on|off>");
        }
        const auto& v = tokens[2];
        if (v == "on" || v == "true" || v == "1" || v == "yes") {
            ctx.session.setFetchCoverArt(true);
            ctx.out << "Cover art fetch/embed: on\n";
            return;
        }
        if (v == "off" || v == "false" || v == "0" || v == "no") {
            ctx.session.setFetchCoverArt(false);
            ctx.out << "Cover art fetch/embed: off\n";
            return;
        }
        throw ParseError("Usage: set coverart <on|off>");
    }
    [[nodiscard]] std::string name() const override { return "set coverart"; }
};

}  // namespace

std::unique_ptr<Command> makeSetOutCommand() { return std::make_unique<SetOutCommand>(); }
std::unique_ptr<Command> makeSetQualityCommand() { return std::make_unique<SetQualityCommand>(); }
std::unique_ptr<Command> makeSetArtistCommand() { return std::make_unique<SetArtistCommand>(); }
std::unique_ptr<Command> makeSetAlbumCommand() { return std::make_unique<SetAlbumCommand>(); }
std::unique_ptr<Command> makeSetExtractorCommand() {
    return std::make_unique<SetExtractorCommand>();
}
std::unique_ptr<Command> makeSetEncoderCommand() { return std::make_unique<SetEncoderCommand>(); }
std::unique_ptr<Command> makeSetCoverCommand() { return std::make_unique<SetCoverCommand>(); }
std::unique_ptr<Command> makeSetCoverArtCommand() { return std::make_unique<SetCoverArtCommand>(); }

}  // namespace optigrab
