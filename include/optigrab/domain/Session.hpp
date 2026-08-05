#pragma once

#include "optigrab/domain/Types.hpp"
#include "optigrab/platform/Platform.hpp"

#include <optional>
#include <string>

namespace optigrab {

class Session {
public:
    void selectDrive(const DriveInfo& drive);
    void clearDriveSelection();
    [[nodiscard]] bool hasSelectedDrive() const;
    [[nodiscard]] const DriveInfo& selectedDrive() const;

    void setDisc(DiscInfo disc);
    void clearDisc();
    [[nodiscard]] bool hasDisc() const;
    [[nodiscard]] const DiscInfo& disc() const;

    void setOutputDirectory(std::string path);
    [[nodiscard]] const std::string& outputDirectory() const;

    void setQuality(QualityPreset quality);
    [[nodiscard]] QualityPreset quality() const;

    void setArtist(std::string artist);
    void setAlbum(std::string album);
    [[nodiscard]] const std::optional<std::string>& artist() const;
    [[nodiscard]] const std::optional<std::string>& album() const;

    // Local cover image path (optional). Tried before network lookup.
    void setCoverPath(std::string path);
    void clearCoverPath();
    [[nodiscard]] const std::optional<std::string>& coverPath() const;

    // When false, skip cover download/embed entirely.
    void setFetchCoverArt(bool enabled);
    [[nodiscard]] bool fetchCoverArt() const;

    // When cover lookup fails: ask | continue | abort (default ask).
    void setCoverMissingPolicy(CoverMissingPolicy policy);
    [[nodiscard]] CoverMissingPolicy coverMissingPolicy() const;

    // Album folder layout under output dir: nested | joined | album (default nested).
    void setFolderLayout(FolderLayout layout);
    [[nodiscard]] FolderLayout folderLayout() const;

    void setExtractor(ExtractorKind kind);
    void setEncoder(EncoderKind kind);
    [[nodiscard]] ExtractorKind extractor() const;
    [[nodiscard]] EncoderKind encoder() const;

    void setRipInProgress(bool value);
    [[nodiscard]] bool ripInProgress() const;

private:
    std::optional<DriveInfo> selectedDrive_;
    std::optional<DiscInfo> disc_;
    std::string outputDirectory_{"."};
    QualityPreset quality_{QualityPreset::V0};
    std::optional<std::string> artist_;
    std::optional<std::string> album_;
    std::optional<std::string> coverPath_;
    bool fetchCoverArt_{true};
    CoverMissingPolicy coverMissingPolicy_{CoverMissingPolicy::Ask};
    FolderLayout folderLayout_{FolderLayout::Nested};
    ExtractorKind extractor_{defaultExtractor()};
    EncoderKind encoder_{EncoderKind::Ffmpeg};
    bool ripInProgress_{false};
};

}  // namespace optigrab
