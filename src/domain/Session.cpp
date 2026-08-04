#include "optigrab/domain/Session.hpp"

#include "optigrab/domain/Errors.hpp"

namespace optigrab {

void Session::selectDrive(const DriveInfo& drive) {
    selectedDrive_ = drive;
    disc_.reset();
}

void Session::clearDriveSelection() {
    selectedDrive_.reset();
    disc_.reset();
}

bool Session::hasSelectedDrive() const { return selectedDrive_.has_value(); }

const DriveInfo& Session::selectedDrive() const {
    if (!selectedDrive_) {
        throw SessionError("No drive selected. Use: select drive <index|path>");
    }
    return *selectedDrive_;
}

void Session::setDisc(DiscInfo disc) { disc_ = std::move(disc); }

void Session::clearDisc() { disc_.reset(); }

bool Session::hasDisc() const { return disc_.has_value(); }

const DiscInfo& Session::disc() const {
    if (!disc_) {
        throw SessionError("No disc loaded. Use: list track (or select a drive with a disc)");
    }
    return *disc_;
}

void Session::setOutputDirectory(std::string path) { outputDirectory_ = std::move(path); }

const std::string& Session::outputDirectory() const { return outputDirectory_; }

void Session::setQuality(QualityPreset quality) { quality_ = quality; }

QualityPreset Session::quality() const { return quality_; }

void Session::setArtist(std::string artist) { artist_ = std::move(artist); }

void Session::setAlbum(std::string album) { album_ = std::move(album); }

const std::optional<std::string>& Session::artist() const { return artist_; }

const std::optional<std::string>& Session::album() const { return album_; }

void Session::setCoverPath(std::string path) { coverPath_ = std::move(path); }

void Session::clearCoverPath() { coverPath_.reset(); }

const std::optional<std::string>& Session::coverPath() const { return coverPath_; }

void Session::setFetchCoverArt(bool enabled) { fetchCoverArt_ = enabled; }

bool Session::fetchCoverArt() const { return fetchCoverArt_; }

void Session::setExtractor(ExtractorKind kind) { extractor_ = kind; }

void Session::setEncoder(EncoderKind kind) { encoder_ = kind; }

ExtractorKind Session::extractor() const { return extractor_; }

EncoderKind Session::encoder() const { return encoder_; }

void Session::setRipInProgress(bool value) { ripInProgress_ = value; }

bool Session::ripInProgress() const { return ripInProgress_; }

}  // namespace optigrab
