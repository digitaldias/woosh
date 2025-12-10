/**
 * @file AudioClip.cpp
 * @brief Implementation of AudioClip with undo support.
 */

#include "AudioClip.h"
#include <filesystem>

AudioClip::AudioClip(std::string path, int sampleRate, int channels, std::vector<float> samples)
    : filePath_(std::move(path))
    , sampleRate_(sampleRate)
    , channels_(channels)
    , samples_(std::move(samples))
{
    displayName_ = std::filesystem::path(filePath_).filename().string();
}

double AudioClip::durationSeconds() const noexcept {
    if (channels_ == 0 || sampleRate_ == 0) return 0.0;
    return static_cast<double>(samples_.size()) / static_cast<double>(channels_ * sampleRate_);
}

size_t AudioClip::frameCount() const noexcept {
    if (channels_ == 0) return 0;
    return samples_.size() / static_cast<size_t>(channels_);
}

void AudioClip::setSamples(std::vector<float> samples) {
    samples_ = std::move(samples);
    modified_ = true;
}

void AudioClip::setFilePath(const std::string& path) {
    filePath_ = path;
    displayName_ = std::filesystem::path(filePath_).filename().string();
}

void AudioClip::updateMetrics(float peakDb, float rmsDb) {
    peakDb_ = peakDb;
    rmsDb_ = rmsDb;
}

void AudioClip::saveOriginal() {
    originalSamples_ = samples_;
    originalPeakDb_ = peakDb_;
    originalRmsDb_ = rmsDb_;
    modified_ = false;
}

void AudioClip::restoreOriginal() {
    if (originalSamples_.empty()) return;

    samples_ = originalSamples_;
    peakDb_ = originalPeakDb_;
    rmsDb_ = originalRmsDb_;
    modified_ = false;
}
