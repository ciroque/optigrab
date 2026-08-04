#include "optigrab/adapters/libcdio/LibcdioParanoiaExtractor.hpp"

#include "optigrab/domain/Errors.hpp"

extern "C" {
#include <cdio/cdio.h>
#include <cdio/paranoia/cdda.h>
#include <cdio/paranoia/paranoia.h>
#include <cdio/sector.h>
}

#include <cstdint>
#include <fstream>

namespace optigrab {
namespace {

#pragma pack(push, 1)
struct WavHeader {
    char riff[4]{'R', 'I', 'F', 'F'};
    uint32_t chunkSize{0};
    char wave[4]{'W', 'A', 'V', 'E'};
    char fmt[4]{'f', 'm', 't', ' '};
    uint32_t fmtSize{16};
    uint16_t audioFormat{1};
    uint16_t numChannels{2};
    uint32_t sampleRate{44100};
    uint32_t byteRate{44100 * 2 * 2};
    uint16_t blockAlign{4};
    uint16_t bitsPerSample{16};
    char data[4]{'d', 'a', 't', 'a'};
    uint32_t dataSize{0};
};
#pragma pack(pop)

}  // namespace

void LibcdioParanoiaExtractor::extractTrack(const std::string& devicePath,
                                            const TrackInfo& track,
                                            const std::filesystem::path& outputWav,
                                            ProgressFn progress) {
    if (!track.audio) {
        throw ExtractError("Track is not audio");
    }

    cdrom_drive_t* d = cdio_cddap_identify(devicePath.c_str(), 1, nullptr);
    if (!d) {
        throw ExtractError("cdio_cddap_identify failed for " + devicePath);
    }
    if (cdio_cddap_open(d) != 0) {
        cdio_cddap_close(d);
        throw ExtractError("cdio_cddap_open failed for " + devicePath);
    }

    cdrom_paranoia_t* p = cdio_paranoia_init(d);
    if (!p) {
        cdio_cddap_close(d);
        throw ExtractError("cdio_paranoia_init failed");
    }

    const lsn_t first = cdio_cddap_track_firstsector(d, static_cast<track_t>(track.number));
    const lsn_t last = cdio_cddap_track_lastsector(d, static_cast<track_t>(track.number));
    if (first < 0 || last < first) {
        cdio_paranoia_free(p);
        cdio_cddap_close(d);
        throw ExtractError("Invalid sector range for track " + std::to_string(track.number));
    }

    cdio_paranoia_modeset(p, PARANOIA_MODE_FULL ^ PARANOIA_MODE_NEVERSKIP);

    std::ofstream out(outputWav, std::ios::binary);
    if (!out) {
        cdio_paranoia_free(p);
        cdio_cddap_close(d);
        throw ExtractError("Cannot write WAV: " + outputWav.string());
    }

    WavHeader hdr;
    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));

    uint32_t dataBytes = 0;
    cdio_paranoia_seek(p, first, SEEK_SET);
    for (lsn_t s = first; s <= last; ++s) {
        int16_t* buf = cdio_paranoia_read(p, nullptr);
        if (!buf) {
            cdio_paranoia_free(p);
            cdio_cddap_close(d);
            throw ExtractError("paranoia read failed at LSN " + std::to_string(s));
        }
        out.write(reinterpret_cast<const char*>(buf), CDIO_CD_FRAMESIZE_RAW);
        dataBytes += CDIO_CD_FRAMESIZE_RAW;
        if (progress && ((s - first) % 75 == 0)) {
            progress("  sector " + std::to_string(s - first) + "/" +
                     std::to_string(last - first + 1));
        }
    }

    hdr.dataSize = dataBytes;
    hdr.chunkSize = 36 + dataBytes;
    out.seekp(0);
    out.write(reinterpret_cast<const char*>(&hdr), sizeof(hdr));
    out.close();

    cdio_paranoia_free(p);
    cdio_cddap_close(d);

    if (progress) {
        progress("Extracted track " + std::to_string(track.number) + " -> " + outputWav.string());
    }
}

}  // namespace optigrab
