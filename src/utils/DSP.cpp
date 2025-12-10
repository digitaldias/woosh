#include "DSP.h"
#include <algorithm>
#include <cmath>

namespace {
constexpr float kEpsilon = 1e-9f;
inline float dbToLinear(float db) { return std::pow(10.0f, db / 20.0f); }
inline float linearToDb(float lin) { return 20.0f * std::log10(std::max(lin, kEpsilon)); }
}

float DSP::computePeakDbFS(const std::vector<float>& samples) {
    float peak = 0.0f;
    for (float s : samples) peak = std::max(peak, std::abs(s));
    return linearToDb(peak);
}

float DSP::computeRMSDb(const std::vector<float>& samples) {
    if (samples.empty()) return -std::numeric_limits<float>::infinity();
    double sumSq = 0.0;
    for (float s : samples) sumSq += static_cast<double>(s) * s;
    double rms = std::sqrt(sumSq / samples.size());
    return static_cast<float>(linearToDb(static_cast<float>(rms)));
}

void DSP::normalizeToPeak(std::vector<float>& samples, float targetDbFS) {
    const float currentPeakDb = computePeakDbFS(samples);
    float gainDb = targetDbFS - currentPeakDb;
    float gain = dbToLinear(gainDb);
    for (float& s : samples) s *= gain;
}

void DSP::normalizeToRMS(std::vector<float>& samples, float targetDb) {
    const float currentRmsDb = computeRMSDb(samples);
    float gainDb = targetDb - currentRmsDb;
    float gain = dbToLinear(gainDb);
    for (float& s : samples) s *= gain;
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



