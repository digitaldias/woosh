#pragma once

#include <vector>

namespace DSP {

/**
 * @brief Types of fade curves available.
 */
enum class FadeType {
    Linear,      ///< Linear ramp
    Exponential, ///< Exponential curve (slow start for fade-in, fast drop for fade-out)
    SCurve       ///< S-curve (slow-fast-slow, smooth transition)
};

[[nodiscard]] float computePeakDbFS(const std::vector<float>& samples);
[[nodiscard]] float computeRMSDb(const std::vector<float>& samples);
void normalizeToPeak(std::vector<float>& samples, float targetDbFS);
void normalizeToRMS(std::vector<float>& samples, float targetDb);
void compressor(std::vector<float>& samples, float thresholdDb, float ratio,
                float attackMs, float releaseMs, float makeupDb,
                int sampleRate, int channels);

/**
 * @brief Apply a fade-in effect to the beginning of the sample buffer.
 * @param samples Audio samples to modify in-place
 * @param fadeLengthSamples Number of samples over which to apply the fade
 * @param fadeType Type of fade curve to use
 */
void applyFadeIn(std::vector<float>& samples, size_t fadeLengthSamples, FadeType fadeType);

/**
 * @brief Apply a fade-out effect to the end of the sample buffer.
 * @param samples Audio samples to modify in-place
 * @param fadeLengthSamples Number of samples over which to apply the fade
 * @param fadeType Type of fade curve to use
 */
void applyFadeOut(std::vector<float>& samples, size_t fadeLengthSamples, FadeType fadeType);

}



