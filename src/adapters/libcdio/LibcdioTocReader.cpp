#include "optigrab/adapters/libcdio/LibcdioTocReader.hpp"

#include "optigrab/domain/Errors.hpp"
#include "optigrab/util/DeviceError.hpp"

#include <cerrno>
#include <fcntl.h>
#include <unistd.h>

extern "C" {
#include <cdio/cdio.h>
#include <cdio/cdtext.h>
#include <cdio/track.h>
}

namespace optigrab {
namespace {

void throwIfCannotOpen(const std::string& devicePath) {
    // Probe open so we can surface errno-based hints (cdio_open is opaque).
    const int fd = ::open(devicePath.c_str(), O_RDONLY | O_NONBLOCK);
    if (fd < 0) {
        throw TocError(describeDeviceFailure(devicePath, errno, "Open optical device"));
    }
    ::close(fd);
}

}  // namespace

DiscInfo LibcdioTocReader::readToc(const std::string& devicePath) {
    throwIfCannotOpen(devicePath);

    CdIo_t* cdio = cdio_open(devicePath.c_str(), DRIVER_DEVICE);
    if (!cdio) {
        cdio = cdio_open(devicePath.c_str(), DRIVER_UNKNOWN);
    }
    if (!cdio) {
        // open() succeeded but libcdio failed — often empty tray / not ready.
        throw TocError(
            "Failed to open device for TOC: " + devicePath +
            "\n  hint: Is an audio CD inserted and ready? Close other apps using the drive.");
    }

    DiscInfo disc;
    disc.devicePath = devicePath;

    const track_t first = cdio_get_first_track_num(cdio);
    const track_t num = cdio_get_num_tracks(cdio);
    if (first == CDIO_INVALID_TRACK || num == CDIO_INVALID_TRACK || num == 0) {
        cdio_destroy(cdio);
        throw TocError("No tracks found on " + devicePath);
    }

    for (track_t t = first; t < first + num; ++t) {
        TrackInfo info;
        info.number = static_cast<int>(t);
        info.startLba = static_cast<std::int64_t>(cdio_get_track_lsn(cdio, t));
        const lsn_t last = cdio_get_track_last_lsn(cdio, t);
        if (info.startLba >= 0 && last >= info.startLba) {
            info.sectors = static_cast<std::int64_t>(last - info.startLba + 1);
        }
        const track_format_t fmt = cdio_get_track_format(cdio, t);
        info.audio = (fmt == TRACK_FORMAT_AUDIO);
        disc.tracks.push_back(std::move(info));
    }

    if (cdtext_t* cdtext = cdio_get_cdtext(cdio)) {
        if (const char* album = cdtext_get_const(cdtext, CDTEXT_FIELD_TITLE, 0)) {
            disc.album = album;
        }
        if (const char* artist = cdtext_get_const(cdtext, CDTEXT_FIELD_PERFORMER, 0)) {
            disc.albumArtist = artist;
        }
        for (auto& track : disc.tracks) {
            if (const char* title =
                    cdtext_get_const(cdtext, CDTEXT_FIELD_TITLE, static_cast<track_t>(track.number))) {
                track.title = title;
            }
            if (const char* artist = cdtext_get_const(cdtext, CDTEXT_FIELD_PERFORMER,
                                                      static_cast<track_t>(track.number))) {
                track.artist = artist;
            }
        }
    }

    cdio_destroy(cdio);
    return disc;
}

}  // namespace optigrab
