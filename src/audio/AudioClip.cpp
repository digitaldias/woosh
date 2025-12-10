#include "AudioClip.h"
#include <filesystem>

AudioClip::AudioClip(std::string path, int sampleRate, int channels, std::vector<float> samples)
    : filePath_(std::move(path)), sampleRate_(sampleRate), channels_(channels), samples_(std::move(samples)) {
    displayName_ = std::filesystem::path(filePath_).filename().string();
}

double AudioClip::durationSeconds() const noexcept {
    if (channels_ == 0 || sampleRate_ == 0) return 0.0;
    return static_cast<double>(samples_.size()) / static_cast<double>(channels_ * sampleRate_);
}

void AudioClip::setSamples(std::vector<float> samples) {
    samples_ = std::move(samples);
}

void AudioClip::setFilePath(const std::string& path) {
    filePath_ = path;
    displayName_ = std::filesystem::path(filePath_).filename().string();
}

void AudioClip::updateMetrics(float peakDb, float rmsDb) {
    peakDb_ = peakDb;
    rmsDb_ = rmsDb;
}



