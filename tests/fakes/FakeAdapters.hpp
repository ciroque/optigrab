#pragma once

#include "optigrab/ports/AudioEncoder.hpp"
#include "optigrab/ports/AudioExtractor.hpp"
#include "optigrab/ports/DriveEnumerator.hpp"
#include "optigrab/ports/MetadataProvider.hpp"
#include "optigrab/ports/TocReader.hpp"

#include <fstream>
#include <utility>

namespace optigrab::test {

class FakeDriveEnumerator : public DriveEnumerator {
public:
    explicit FakeDriveEnumerator(std::vector<DriveInfo> drives) : drives_(std::move(drives)) {}
    std::vector<DriveInfo> listDrives() override { return drives_; }

private:
    std::vector<DriveInfo> drives_;
};

class FakeTocReader : public TocReader {
public:
    explicit FakeTocReader(DiscInfo disc) : disc_(std::move(disc)) {}
    DiscInfo readToc(const std::string& devicePath) override {
        disc_.devicePath = devicePath;
        return disc_;
    }

private:
    DiscInfo disc_;
};

class FakeExtractor : public AudioExtractor {
public:
    void extractTrack(const std::string&, const TrackInfo& track,
                      const std::filesystem::path& outputWav, ProgressFn) override {
        lastTrack_ = track.number;
        // Minimal valid-enough WAV for ffmpeg tests isn't needed; write dummy bytes.
        std::ofstream out(outputWav, std::ios::binary);
        const char data[44 + 4]{};
        out.write(data, sizeof(data));
        wrote_ = outputWav;
    }
    std::string name() const override { return "fake-extractor"; }

    int lastTrack_{0};
    std::filesystem::path wrote_;
};

class FakeEncoder : public AudioEncoder {
public:
    void encode(const std::filesystem::path& inputWav, const std::filesystem::path& outputMp3,
                const Tags& tags, QualityPreset, ProgressFn) override {
        lastTags_ = tags;
        lastInput_ = inputWav;
        std::filesystem::create_directories(outputMp3.parent_path());
        std::ofstream out(outputMp3, std::ios::binary);
        out << "ID3fake";
        wrote_ = outputMp3;
    }
    std::string name() const override { return "fake-encoder"; }

    Tags lastTags_;
    std::filesystem::path lastInput_;
    std::filesystem::path wrote_;
};

class FakeMetadata : public MetadataProvider {
public:
    void enrich(DiscInfo& disc) override {
        disc.album = "Test Album";
        disc.albumArtist = "Test Artist";
        for (auto& t : disc.tracks) {
            if (t.audio && t.title.empty()) {
                t.title = "Song " + std::to_string(t.number);
                t.artist = "Test Artist";
            }
        }
    }
    std::string name() const override { return "fake-meta"; }
};

inline DiscInfo makeTwoTrackDisc() {
    DiscInfo d;
    d.tracks.push_back(TrackInfo{1, 0, 100, true, "", ""});
    d.tracks.push_back(TrackInfo{2, 100, 200, true, "", ""});
    return d;
}

}  // namespace optigrab::test
