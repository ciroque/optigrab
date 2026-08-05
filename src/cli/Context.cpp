#include "optigrab/cli/Context.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/services/Filename.hpp"

namespace optigrab {
namespace {

void resolveArtistAlbum(const Session& session, std::string& artist, std::string& album) {
    if (session.artist()) {
        artist = *session.artist();
    } else if (session.hasDisc() && session.disc().albumArtist) {
        artist = *session.disc().albumArtist;
    } else {
        artist = "Unknown Artist";
    }

    if (session.album()) {
        album = *session.album();
    } else if (session.hasDisc() && session.disc().album) {
        album = *session.disc().album;
    } else {
        album = "Unknown Album";
    }
}

}  // namespace

void Context::rebindLogFile() {
    if (!session.logPathDir()) {
        log.clearSecondaryFile();
        return;
    }

    std::string artist;
    std::string album;
    resolveArtistAlbum(session, artist, album);

    const auto path = buildLogFilePath(*session.logPathDir(), artist, album);
    std::string err;
    if (!log.tryOpenSecondaryFile(path, err)) {
        throw OptigrabError(err);
    }
    log.info("logging also to " + path.string());
}

void Context::setLogPath(const std::string& dir) {
    if (dir.empty()) {
        throw OptigrabError("Log path directory must not be empty");
    }
    session.setLogPathDir(dir);
    rebindLogFile();
}

void Context::clearLogPath() {
    const auto prev = session.logPathDir();
    session.clearLogPathDir();
    log.clearSecondaryFile();
    if (prev) {
        log.info("stopped logging to directory " + *prev);
    }
}

}  // namespace optigrab
