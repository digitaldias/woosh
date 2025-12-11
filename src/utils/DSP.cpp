#include "DSP.h"
#include <algorithm>
#include <cmath>
#include <execution>
#include <numeric>

namespace {
constexpr float kEpsilon = 1e-9f;
inline float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }
inline float linearToDb(float lin) { return 20.0f * std::log10(std::max(lin, kEpsilon)); }

// Threshold for using parallel execution (below this, overhead outweighs benefit)
constexpr size_t kParallelThreshold = 10000;
}

float DSP::computePeakDbFS(const std::vector<float>& samples) {
    if (samples.empty()) return -std::numeric_limits<float>::infinity();
    
    float peak = 0.0f;
    if (samples.size() >= kParallelThreshold) {
        // Parallel reduction for large buffers
        peak = std::transform_reduce(
            std::execution::par_unseq,
            samples.begin(), samples.end(),
            0.0f,
            [](float a, float b) { return std::max(a, b); },
            [](float s) { return std::abs(s); }
        );
    } else {
        for (float s : samples) peak = std::max(peak, std::abs(s));
    }
    return linearToDb(peak);
}

float DSP::computeRMSDb(const std::vector<float>& samples) {
    if (samples.empty()) return -std::numeric_limits<float>::infinity();
    
    double sumSq = 0.0;
    if (samples.size() >= kParallelThreshold) {
        // Parallel reduction for large buffers
        sumSq = std::transform_reduce(
            std::execution::par_unseq,
            samples.begin(), samples.end(),
            0.0,
            std::plus<double>{},
            [](float s) { return static_cast<double>(s) * s; }
        );
    } else {
        for (float s : samples) sumSq += static_cast<double>(s) * s;
    }
    double rms = std::sqrt(sumSq / samples.size());
    return static_cast<float>(linearToDb(static_cast<float>(rms)));
}

void DSP::normalizeToPeak(std::vector<float>& samples, float targetDbFS) {
    const float currentPeakDb = computePeakDbFS(samples);
    float gainDb = targetDbFS - currentPeakDb;
    float gain = dbToLinear(gainDb);
    
    if (samples.size() >= kParallelThreshold) {
        // Parallel gain application for large buffers
        std::for_each(std::execution::par_unseq, samples.begin(), samples.end(),
            [gain](float& s) { s *= gain; });
    } else {
        for (float& s : samples) s *= gain;
    }
}

void DSP::normalizeToRMS(std::vector<float>& samples, float targetDb) {
    const float currentRmsDb = computeRMSDb(samples);
    float gainDb = targetDb - currentRmsDb;
    float gain = dbToLinear(gainDb);
    
    if (samples.size() >= kParallelThreshold) {
        // Parallel gain application for large buffers
        std::for_each(std::execution::par_unseq, samples.begin(), samples.end(),
            [gain](float& s) { s *= gain; });
    } else {
        for (float& s : samples) s *= gain;
    }
}

void DSP::compressor(std::vector<float>& samples, float thresholdDb, float ratio,
                     float attackMs, float releaseMs, float makeupDb,
                     int sampleRate, int channels) {
    if (sampleRate <= 0 || channels <= 0) return;
    float thresholdLin = dbToLinear(thresholdDb);
    float makeupLin = dbToLinear(makeupDb);
    float attackCoeff = std::exp(-1.0f / (0.001f * attackMs * sampleRate));
    float releaseCoeff = std::exp(-1.0f / (0.001f * releaseMs * sampleRate));
    float env = 0.0f;
    for (size_t i = 0; i < samples.size(); i += channels) {
        float framePeak = 0.0f;
        for (int c = 0; c < channels; ++c) framePeak = std::max(framePeak, std::abs(samples[i + c]));
        env = framePeak > env ? attackCoeff * (env - framePeak) + framePeak : releaseCoeff * (env - framePeak) + framePeak;
        float gain = 1.0f;
        if (env > thresholdLin) {
            float overDb = linearToDb(env) - thresholdDb;
            float reducedDb = overDb / ratio;
            float gainDb = -(overDb - reducedDb);
            gain = dbToLinear(gainDb);
        }
        for (int c = 0; c < channels; ++c) samples[i + c] *= gain * makeupLin;
    }
}

namespace {
/**
 * @brief Compute fade gain for a given position.
 * @param position Current sample index within the fade region
 * @param fadeLength Total length of the fade in samples
 * @param fadeType Type of fade curve
 * @param isFadeIn True for fade-in (0→1), false for fade-out (1→0)
 * @return Gain value between 0.0 and 1.0
 */
float computeFadeGain(size_t position, size_t fadeLength, DSP::FadeType fadeType, bool isFadeIn) {
    if (fadeLength == 0) return 1.0f;
    
    // Normalized position (0.0 to 1.0)
    float t = static_cast<float>(position) / static_cast<float>(fadeLength);
    t = std::clamp(t, 0.0f, 1.0f);
    
    float gain = 0.0f;
    
    switch (fadeType) {
        case DSP::FadeType::Linear:
            gain = t;
            break;
            
        case DSP::FadeType::Exponential:
            // Attempt an exponential-like curve: 
            // For fade-in: slow start, fast finish
            // gain = t^2 gives a nice curve
            gain = t * t;
            break;
            
        case DSP::FadeType::SCurve:
            // Attempt smoothstep S-curve: 3t^2 - 2t^3
            gain = t * t * (3.0f - 2.0f * t);
            break;
    }
    
    // For fade-out, invert the gain
    if (!isFadeIn) {
        gain = 1.0f - gain;
    }
    
    return gain;
}
} // anonymous namespace

void DSP::applyFadeIn(std::vector<float>& samples, size_t fadeLengthSamples, FadeType fadeType) {
    if (samples.empty() || fadeLengthSamples == 0) return;
    
    // Clamp fade length to buffer size
    size_t actualFadeLength = std::min(fadeLengthSamples, samples.size());
    
    for (size_t i = 0; i < actualFadeLength; ++i) {
        float gain = computeFadeGain(i, actualFadeLength, fadeType, true);
        samples[i] *= gain;
    }
}

void DSP::applyFadeOut(std::vector<float>& samples, size_t fadeLengthSamples, FadeType fadeType) {
    if (samples.empty() || fadeLengthSamples == 0) return;
    
    // Clamp fade length to buffer size
    size_t actualFadeLength = std::min(fadeLengthSamples, samples.size());
    size_t fadeStart = samples.size() - actualFadeLength;
    
    for (size_t i = 0; i < actualFadeLength; ++i) {
        float gain = computeFadeGain(i, actualFadeLength, fadeType, false);
        samples[fadeStart + i] *= gain;
    }
}



