#include "optigrab/domain/DiscId.hpp"

#include <array>
#include <cstdio>
#include <cstdint>
#include <cstring>
#include <vector>

namespace optigrab {
namespace {

// SHA-1 used for MusicBrainz Disc ID (same digest as libdiscid).
class Sha1 {
public:
    Sha1() { reset(); }

    void update(const std::uint8_t* data, std::size_t len) {
        for (std::size_t i = 0; i < len; ++i) {
            data_[datalen_] = data[i];
            ++datalen_;
            if (datalen_ == 64) {
                transform();
                bitlen_ += 512;
                datalen_ = 0;
            }
        }
    }

    void update(const char* ascii) {
        update(reinterpret_cast<const std::uint8_t*>(ascii), std::strlen(ascii));
    }

    std::array<std::uint8_t, 20> final() {
        std::array<std::uint8_t, 20> hash{};
        const std::uint32_t i = datalen_;

        if (datalen_ < 56) {
            data_[datalen_++] = 0x80;
            while (datalen_ < 56) {
                data_[datalen_++] = 0x00;
            }
        } else {
            data_[datalen_++] = 0x80;
            while (datalen_ < 64) {
                data_[datalen_++] = 0x00;
            }
            transform();
            std::memset(data_.data(), 0, 56);
        }

        bitlen_ += static_cast<std::uint64_t>(i) * 8;
        data_[63] = static_cast<std::uint8_t>(bitlen_);
        data_[62] = static_cast<std::uint8_t>(bitlen_ >> 8);
        data_[61] = static_cast<std::uint8_t>(bitlen_ >> 16);
        data_[60] = static_cast<std::uint8_t>(bitlen_ >> 24);
        data_[59] = static_cast<std::uint8_t>(bitlen_ >> 32);
        data_[58] = static_cast<std::uint8_t>(bitlen_ >> 40);
        data_[57] = static_cast<std::uint8_t>(bitlen_ >> 48);
        data_[56] = static_cast<std::uint8_t>(bitlen_ >> 56);
        transform();

        for (std::uint32_t j = 0; j < 4; ++j) {
            hash[j] = (state_[0] >> (24 - j * 8)) & 0xff;
            hash[j + 4] = (state_[1] >> (24 - j * 8)) & 0xff;
            hash[j + 8] = (state_[2] >> (24 - j * 8)) & 0xff;
            hash[j + 12] = (state_[3] >> (24 - j * 8)) & 0xff;
            hash[j + 16] = (state_[4] >> (24 - j * 8)) & 0xff;
        }
        return hash;
    }

private:
    void reset() {
        datalen_ = 0;
        bitlen_ = 0;
        state_ = {0x67452301, 0xefcdab89, 0x98badcfe, 0x10325476, 0xc3d2e1f0};
    }

    static std::uint32_t rotl(std::uint32_t a, std::uint32_t b) {
        return (a << b) | (a >> (32 - b));
    }

    void transform() {
        std::uint32_t m[80];
        for (std::uint32_t i = 0, j = 0; i < 16; ++i, j += 4) {
            m[i] = (data_[j] << 24) | (data_[j + 1] << 16) | (data_[j + 2] << 8) | (data_[j + 3]);
        }
        for (std::uint32_t i = 16; i < 80; ++i) {
            m[i] = rotl(m[i - 3] ^ m[i - 8] ^ m[i - 14] ^ m[i - 16], 1);
        }

        std::uint32_t a = state_[0];
        std::uint32_t b = state_[1];
        std::uint32_t c = state_[2];
        std::uint32_t d = state_[3];
        std::uint32_t e = state_[4];

        for (std::uint32_t i = 0; i < 80; ++i) {
            std::uint32_t f = 0;
            std::uint32_t k = 0;
            if (i < 20) {
                f = (b & c) | ((~b) & d);
                k = 0x5a827999;
            } else if (i < 40) {
                f = b ^ c ^ d;
                k = 0x6ed9eba1;
            } else if (i < 60) {
                f = (b & c) | (b & d) | (c & d);
                k = 0x8f1bbcdc;
            } else {
                f = b ^ c ^ d;
                k = 0xca62c1d6;
            }
            const std::uint32_t temp = rotl(a, 5) + f + e + k + m[i];
            e = d;
            d = c;
            c = rotl(b, 30);
            b = a;
            a = temp;
        }

        state_[0] += a;
        state_[1] += b;
        state_[2] += c;
        state_[3] += d;
        state_[4] += e;
    }

    std::array<std::uint8_t, 64> data_{};
    std::uint32_t datalen_{0};
    std::uint64_t bitlen_{0};
    std::array<std::uint32_t, 5> state_{};
};

// RFC822/libdiscid Base64 of SHA-1 with MusicBrainz alphabet:
//   + → .   / → _   = → -
// Always 28 characters for a 20-byte digest.
std::string mbBase64(const std::array<std::uint8_t, 20>& digest) {
    static const char kTbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789+/";
    std::string out;
    out.reserve(28);

    int i = 0;
    for (; i + 2 < 20; i += 3) {
        const unsigned n = (static_cast<unsigned>(digest[static_cast<std::size_t>(i)]) << 16) |
                           (static_cast<unsigned>(digest[static_cast<std::size_t>(i + 1)]) << 8) |
                           static_cast<unsigned>(digest[static_cast<std::size_t>(i + 2)]);
        out.push_back(kTbl[(n >> 18) & 63]);
        out.push_back(kTbl[(n >> 12) & 63]);
        out.push_back(kTbl[(n >> 6) & 63]);
        out.push_back(kTbl[n & 63]);
    }
    // 20 % 3 == 2 → two leftover bytes → three chars + one pad
    const unsigned n = (static_cast<unsigned>(digest[18]) << 16) |
                       (static_cast<unsigned>(digest[19]) << 8);
    out.push_back(kTbl[(n >> 18) & 63]);
    out.push_back(kTbl[(n >> 12) & 63]);
    out.push_back(kTbl[(n >> 6) & 63]);
    out.push_back('=');

    for (char& c : out) {
        if (c == '+') {
            c = '.';
        } else if (c == '/') {
            c = '_';
        } else if (c == '=') {
            c = '-';
        }
    }
    return out;
}

// libdiscid hashes *ASCII hex strings*, not raw binary:
//   "%02X" first, "%02X" last, then 100 × "%08X" offsets[0..99]
// offsets[0] = lead-out (sectors), offsets[n] = start of track n (CD frames).
std::string hashDiscId(int first, int last, const std::array<std::uint32_t, 100>& offsets) {
    Sha1 sha;
    char tmp[16];

    std::snprintf(tmp, sizeof(tmp), "%02X", first);
    sha.update(tmp);
    std::snprintf(tmp, sizeof(tmp), "%02X", last);
    sha.update(tmp);

    for (std::uint32_t off : offsets) {
        std::snprintf(tmp, sizeof(tmp), "%08X", static_cast<unsigned>(off));
        sha.update(tmp);
    }
    return mbBase64(sha.final());
}

}  // namespace

std::optional<std::string> computeMusicBrainzDiscId(const DiscInfo& disc) {
    std::vector<const TrackInfo*> audio;
    for (const auto& t : disc.tracks) {
        if (t.audio) {
            audio.push_back(&t);
        }
    }
    if (audio.empty()) {
        return std::nullopt;
    }

    const int first = audio.front()->number;
    const int last = audio.back()->number;
    if (first < 1 || last < first || last > 99) {
        return std::nullopt;
    }

    // Frame offset = LBA + 150 (2-second pregap).
    // offsets[0] = lead-out frame; offsets[track] = track start frame.
    std::array<std::uint32_t, 100> offsets{};
    for (const auto* t : audio) {
        if (t->number < 1 || t->number > 99) {
            return std::nullopt;
        }
        offsets[static_cast<std::size_t>(t->number)] =
            static_cast<std::uint32_t>(t->startLba + 150);
    }
    const auto* lastTrack = audio.back();
    // Lead-out = end of last audio track in frames.
    offsets[0] = static_cast<std::uint32_t>(lastTrack->startLba + lastTrack->sectors + 150);

    return hashDiscId(first, last, offsets);
}

// Test/helper: compute from raw first/last/leadout/track frame offsets (libdiscid layout).
std::optional<std::string> computeMusicBrainzDiscIdFromOffsets(
    int first, int last, std::uint32_t leadOutFrames,
    const std::vector<std::uint32_t>& trackStartFrames) {
    if (first < 1 || last < first || last > 99) {
        return std::nullopt;
    }
    if (static_cast<int>(trackStartFrames.size()) != (last - first + 1)) {
        return std::nullopt;
    }
    std::array<std::uint32_t, 100> offsets{};
    offsets[0] = leadOutFrames;
    for (int t = first; t <= last; ++t) {
        offsets[static_cast<std::size_t>(t)] = trackStartFrames[static_cast<std::size_t>(t - first)];
    }
    return hashDiscId(first, last, offsets);
}

}  // namespace optigrab
