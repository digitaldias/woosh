#pragma once

#include <vector>

namespace DSP {
float computePeakDbFS(const std::vector<float>& samples);
float computeRMSDb(const std::vector<float>& samples);
void normalizeToPeak(std::vector<float>& samples, float targetDbFS);
void normalizeToRMS(std::vector<float>& samples, float targetDb);
void compressor(std::vector<float>& samples, float thresholdDb, float ratio,
                float attackMs, float releaseMs, float makeupDb,
                int sampleRate, int channels);
}



