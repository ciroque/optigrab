#include "optigrab/domain/DiscId.hpp"

#include <array>
#include <cstdint>
#include <cstring>
#include <vector>

namespace optigrab {
namespace {

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

// MusicBrainz alphabet: + → .  / → _
std::string mbBase64(const std::array<std::uint8_t, 20>& hash) {
    static const char tbl[] =
        "ABCDEFGHIJKLMNOPQRSTUVWXYZabcdefghijklmnopqrstuvwxyz0123456789._";
    std::string out(28, '\0');
    unsigned char in[21]{};
    std::memcpy(in, hash.data(), 20);
    int o = 0;
    for (int i = 0; i < 18; i += 3) {
        const unsigned n = (in[i] << 16) | (in[i + 1] << 8) | in[i + 2];
        out[static_cast<std::size_t>(o++)] = tbl[(n >> 18) & 63];
        out[static_cast<std::size_t>(o++)] = tbl[(n >> 12) & 63];
        out[static_cast<std::size_t>(o++)] = tbl[(n >> 6) & 63];
        out[static_cast<std::size_t>(o++)] = tbl[n & 63];
    }
    const unsigned n = (in[18] << 16) | (in[19] << 8);
    out[static_cast<std::size_t>(o++)] = tbl[(n >> 18) & 63];
    out[static_cast<std::size_t>(o++)] = tbl[(n >> 12) & 63];
    out[static_cast<std::size_t>(o++)] = tbl[(n >> 6) & 63];
    out[static_cast<std::size_t>(o++)] = tbl[n & 63];
    return out;
}

void putBe32(std::uint8_t* p, std::uint32_t v) {
    p[0] = static_cast<std::uint8_t>((v >> 24) & 0xff);
    p[1] = static_cast<std::uint8_t>((v >> 16) & 0xff);
    p[2] = static_cast<std::uint8_t>((v >> 8) & 0xff);
    p[3] = static_cast<std::uint8_t>(v & 0xff);
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

    std::array<std::uint32_t, 100> offsets{};
    for (const auto* t : audio) {
        if (t->number < 1 || t->number > 99) {
            return std::nullopt;
        }
        offsets[static_cast<std::size_t>(t->number)] =
            static_cast<std::uint32_t>(t->startLba + 150);
    }
    const auto* lastTrack = audio.back();
    offsets[0] = static_cast<std::uint32_t>(lastTrack->startLba + lastTrack->sectors + 150);

    Sha1 sha;
    const std::uint8_t firstB = static_cast<std::uint8_t>(first);
    const std::uint8_t lastB = static_cast<std::uint8_t>(last);
    sha.update(&firstB, 1);
    sha.update(&lastB, 1);
    for (std::uint32_t off : offsets) {
        std::uint8_t be[4];
        putBe32(be, off);
        sha.update(be, 4);
    }
    return mbBase64(sha.final());
}

}  // namespace optigrab
