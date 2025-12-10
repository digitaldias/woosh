/**
 * @file AudioClip.h
 * @brief Represents an audio clip with sample data and metadata.
 *
 * Supports undo of processing operations by storing a copy of the original
 * samples that can be restored.
 */

#pragma once

#include <string>
#include <vector>

/**
 * @class AudioClip
 * @brief In-memory representation of an audio file.
 *
 * Stores interleaved float samples, sample rate, channel count, and derived
 * metrics (peak dB, RMS dB). Supports saving and restoring original samples
 * for undo functionality.
 */
class AudioClip {
public:
    AudioClip() = default;

    /**
     * @brief Construct a clip from loaded audio data.
     * @param path Source file path.
     * @param sampleRate Sample rate in Hz.
     * @param channels Number of audio channels.
     * @param samples Interleaved float samples.
     */
    AudioClip(std::string path, int sampleRate, int channels, std::vector<float> samples);

    // --- Accessors ---

    const std::string& filePath() const noexcept { return filePath_; }
    const std::string& displayName() const noexcept { return displayName_; }
    int sampleRate() const noexcept { return sampleRate_; }
    int channels() const noexcept { return channels_; }
    const std::vector<float>& samples() const noexcept { return samples_; }
    std::vector<float>& samplesMutable() noexcept { return samples_; }

    /** @brief Duration in seconds. */
    double durationSeconds() const noexcept;

    /** @brief Total number of sample frames (samples / channels). */
    size_t frameCount() const noexcept;

    float peakDb() const noexcept { return peakDb_; }
    float rmsDb() const noexcept { return rmsDb_; }

    // --- Mutators ---

    void setSamples(std::vector<float> samples);
    void setFilePath(const std::string& path);
    void updateMetrics(float peakDb, float rmsDb);

    // --- Undo support ---

    /**
     * @brief Save current samples as the original (for undo).
     *
     * Call this after loading, before any processing. Also saves current metrics.
     */
    void saveOriginal();

    /**
     * @brief Restore samples to the saved original state.
     *
     * Reverts any processing (normalize, compress, trim) back to the
     * originally loaded samples.
     */
    void restoreOriginal();

    /**
     * @brief Check if original samples are available for restore.
     * @return True if saveOriginal() was called and restore is possible.
     */
    bool hasOriginal() const noexcept { return !originalSamples_.empty(); }

    /**
     * @brief Check if clip has been modified since loading.
     * @return True if samples differ from original.
     */
    bool isModified() const noexcept { return modified_; }

    /**
     * @brief Mark the clip as modified (called after processing).
     */
    void setModified(bool modified) noexcept { modified_ = modified; }

private:
    std::string filePath_;
    std::string displayName_;
    int sampleRate_{44100};
    int channels_{2};
    std::vector<float> samples_;
    float peakDb_{0.0f};
    float rmsDb_{0.0f};

    // Undo support: original state
    std::vector<float> originalSamples_;
    float originalPeakDb_{0.0f};
    float originalRmsDb_{0.0f};
    bool modified_{false};
};
