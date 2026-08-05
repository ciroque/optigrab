#include "optigrab/cli/Args.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/platform/Platform.hpp"

#include <sstream>

namespace optigrab {
namespace {

QualityPreset parseQualityFlag(const std::string& s) {
    if (s == "V0" || s == "v0") return QualityPreset::V0;
    if (s == "V2" || s == "v2") return QualityPreset::V2;
    if (s == "192") return QualityPreset::Cbr192;
    if (s == "256") return QualityPreset::Cbr256;
    if (s == "320") return QualityPreset::Cbr320;
    throw ParseError("Unknown --quality value: " + s + " (V0, V2, 192, 256, 320)");
}

ExtractorKind parseExtractorFlag(const std::string& s) {
    if (s == "ffmpeg") return ExtractorKind::Ffmpeg;
#ifndef _WIN32
    if (s == "cdparanoia") return ExtractorKind::Cdparanoia;
    if (s == "libcdio" || s == "libcdio_paranoia" || s == "paranoia") {
        return ExtractorKind::LibcdioParanoia;
    }
#endif
    throw ParseError(std::string("Unknown --extractor value: ") + s);
}

EncoderKind parseEncoderFlag(const std::string& s) {
    if (s == "ffmpeg") return EncoderKind::Ffmpeg;
    throw ParseError("Unknown --encoder value: " + s + " (ffmpeg)");
}

FolderLayout parseFolderLayoutFlag(const std::string& s) {
    if (s == "nested") return FolderLayout::Nested;
    if (s == "joined") return FolderLayout::Joined;
    if (s == "album") return FolderLayout::Album;
    throw ParseError("Unknown --folder-layout value: " + s + " (nested|joined|album)");
}

std::string requireValue(const std::vector<std::string>& argv, std::size_t& i,
                         const std::string& flag) {
    if (i + 1 >= argv.size()) {
        throw ParseError("Missing value for " + flag);
    }
    return argv[++i];
}

}  // namespace

std::string usageText() {
    std::ostringstream oss;
    oss << "optigrab — optical disc grabber\n\n"
        << "Usage:\n"
        << "  optigrab                          Interactive REPL\n"
        << "  optigrab [options] <verb> <noun> [args...]\n"
        << "  optigrab --help | --version\n\n"
        << "Options:\n"
        << "  --drive <index|path>   Select drive (e.g. 0 or /dev/sr0 or D:)\n"
        << "  --out <dir>            Output directory\n"
        << "  --artist <name>        Album artist override\n"
        << "  --album <name>         Album title override\n"
        << "  --cover <image>        Local cover image (skip network if set)\n"
        << "  --no-cover             Disable cover art fetch/embed\n"
        << "  --cover-missing <p>    ask|continue|abort when no cover (default: ask)\n"
        << "  --folder-layout <l>    nested|joined|album (default: nested)\n"
        << "                           nested  out/Artist/Album/track.mp3\n"
        << "                           joined  out/Artist - Album/track.mp3\n"
        << "                           album   out/Album/track.mp3\n"
        << "  --log-level <level>    trace|debug|info|warn|error|fatal|off (default: info)\n"
        << "  --quality <preset>     V0 | V2 | 192 | 256 | 320\n";
#ifdef _WIN32
    oss << "  --extractor <name>     ffmpeg\n";
#else
    oss << "  --extractor <name>     ffmpeg | cdparanoia | libcdio\n";
#endif
    oss << "  --encoder <name>       ffmpeg\n"
        << "  -h, --help             Show this help\n"
        << "  --version              Show version\n\n"
        << "Examples:\n"
        << "  optigrab list drive\n"
        << "  optigrab --drive 0 list track\n"
        << "  optigrab --drive 0 --out ~/Music --artist \"The Band\" --album \"Live\" "
           "rip track all\n"
        << "\nPlatform default extractor: " << toString(defaultExtractor()) << " ("
        << platformName() << ")\n";
    return oss.str();
}

LaunchArgs parseLaunchArgs(const std::vector<std::string>& argv) {
    LaunchArgs out;
    std::size_t i = 0;
    while (i < argv.size()) {
        const std::string& a = argv[i];
        if (a == "--") {
            ++i;
            break;
        }
        if (a == "-h" || a == "--help") {
            out.showHelp = true;
            ++i;
            continue;
        }
        if (a == "--version") {
            out.showVersion = true;
            ++i;
            continue;
        }
        if (a == "--drive") {
            out.drive = requireValue(argv, i, a);
            ++i;
            continue;
        }
        if (a == "--out") {
            out.outDir = requireValue(argv, i, a);
            ++i;
            continue;
        }
        if (a == "--artist") {
            out.artist = requireValue(argv, i, a);
            ++i;
            continue;
        }
        if (a == "--album") {
            out.album = requireValue(argv, i, a);
            ++i;
            continue;
        }
        if (a == "--cover") {
            out.coverPath = requireValue(argv, i, a);
            ++i;
            continue;
        }
        if (a == "--no-cover") {
            out.fetchCoverArt = false;
            ++i;
            continue;
        }
        if (a == "--cover-missing" || a == "--covermissing") {
            const auto v = requireValue(argv, i, a);
            if (v == "ask") {
                out.coverMissing = CoverMissingPolicy::Ask;
            } else if (v == "continue" || v == "cont") {
                out.coverMissing = CoverMissingPolicy::Continue;
            } else if (v == "abort" || v == "stop" || v == "fail") {
                out.coverMissing = CoverMissingPolicy::Abort;
            } else {
                throw ParseError("Unknown --cover-missing value: " + v + " (ask|continue|abort)");
            }
            ++i;
            continue;
        }
        if (a == "--log-level" || a == "--loglevel") {
            const auto v = requireValue(argv, i, a);
            const auto level = parseLogLevel(v);
            if (!level) {
                throw ParseError("Unknown --log-level value: " + v +
                                 " (trace|debug|info|warn|error|fatal|off)");
            }
            out.logLevel = *level;
            ++i;
            continue;
        }
        if (a == "--folder-layout" || a == "--folderlayout") {
            out.folderLayout = parseFolderLayoutFlag(requireValue(argv, i, a));
            ++i;
            continue;
        }
        if (a == "--quality") {
            out.quality = parseQualityFlag(requireValue(argv, i, a));
            ++i;
            continue;
        }
        if (a == "--extractor") {
            out.extractor = parseExtractorFlag(requireValue(argv, i, a));
            ++i;
            continue;
        }
        if (a == "--encoder") {
            out.encoder = parseEncoderFlag(requireValue(argv, i, a));
            ++i;
            continue;
        }
        if (!a.empty() && a[0] == '-') {
            throw ParseError("Unknown option: " + a + " (try --help)");
        }
        break;  // start of command tokens
    }

    while (i < argv.size()) {
        out.command.push_back(argv[i++]);
    }

    out.interactive = out.command.empty() && !out.showHelp && !out.showVersion;
    return out;
}

}  // namespace optigrab
