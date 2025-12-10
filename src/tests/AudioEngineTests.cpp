#include <cassert>
#include <cmath>
#include <vector>
#include "utils/DSP.h"
#include "audio/AudioClip.h"
#include "audio/AudioEngine.h"

static std::vector<float> makeSine(float freq, int sr, int frames, int channels) {
    std::vector<float> data(frames * channels);
    for (int i = 0; i < frames; ++i) {
        float v = std::sin(2.0f * 3.1415926f * freq * i / sr) * 0.5f;
        for (int c = 0; c < channels; ++c) data[i * channels + c] = v;
    }
    return data;
}

static void testNormalizePeak() {
    auto samples = makeSine(440.0f, 48000, 48000, 2);
    [[maybe_unused]] float before = DSP::computePeakDbFS(samples);
    DSP::normalizeToPeak(samples, -1.0f);
    [[maybe_unused]] float after = DSP::computePeakDbFS(samples);
    assert(after > -1.1f && after < -0.9f);
    assert(after > before);
}

static void testTrim() {
    AudioClip clip("test.wav", 48000, 2, makeSine(440.0f, 48000, 48000, 2));
    AudioEngine engine;
    engine.trim(clip, 0.0f, 0.5f);
    assert(clip.durationSeconds() > 0.49 && clip.durationSeconds() < 0.51);
}

int main() {
    testNormalizePeak();
    testTrim();
    return 0;
}



