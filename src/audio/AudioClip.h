#pragma once

#include <string>
#include <vector>

class AudioClip {
public:
    AudioClip() = default;
    AudioClip(std::string path, int sampleRate, int channels, std::vector<float> samples);

    const std::string& filePath() const noexcept { return filePath_; }
    const std::string& displayName() const noexcept { return displayName_; }
    int sampleRate() const noexcept { return sampleRate_; }
    int channels() const noexcept { return channels_; }
    const std::vector<float>& samples() const noexcept { return samples_; }
    std::vector<float>& samplesMutable() noexcept { return samples_; }

    double durationSeconds() const noexcept;
    float peakDb() const noexcept { return peakDb_; }
    float rmsDb() const noexcept { return rmsDb_; }

    void setSamples(std::vector<float> samples);
    void setFilePath(const std::string& path);
    void updateMetrics(float peakDb, float rmsDb);

private:
    std::string filePath_;
    std::string displayName_;
    int sampleRate_{44100};
    int channels_{2};
    std::vector<float> samples_;
    float peakDb_{0.0f};
    float rmsDb_{0.0f};
};


