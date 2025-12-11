/**
 * @file DSPTests.cpp
 * @brief Unit tests for DSP utility functions.
 */

#include <cassert>
#include <cmath>
#include <vector>
#include <limits>
#include "utils/DSP.h"

// ============================================================================
// Helper functions
// ============================================================================

static std::vector<float> makeSilence(size_t samples) {
    return std::vector<float>(samples, 0.0f);
}

static std::vector<float> makeConstant(size_t samples, float value) {
    return std::vector<float>(samples, value);
}

static std::vector<float> makeSine(float freq, int sampleRate, int frames, int channels, float amplitude = 1.0f) {
    std::vector<float> data(static_cast<size_t>(frames) * static_cast<size_t>(channels));
    constexpr float kTwoPi = 2.0f * 3.14159265358979f;
    for (int i = 0; i < frames; ++i) {
        float v = std::sin(kTwoPi * freq * static_cast<float>(i) / static_cast<float>(sampleRate)) * amplitude;
        for (int c = 0; c < channels; ++c) {
            data[static_cast<size_t>(i) * static_cast<size_t>(channels) + static_cast<size_t>(c)] = v;
        }
    }
    return data;
}

static bool approxEqual(float a, float b, float tolerance = 0.1f) {
    return std::abs(a - b) < tolerance;
}

// ============================================================================
// computePeakDbFS tests
// ============================================================================

static void testComputePeakDbFS_withSilence() {
    std::vector<float> silence = makeSilence(1000);
    float peak = DSP::computePeakDbFS(silence);
    // Silence should return very negative dB (approaching -infinity)
    assert(peak < -90.0f);
}

static void testComputePeakDbFS_withFullScale() {
    std::vector<float> fullScale = {1.0f, -1.0f, 0.5f, -0.5f};
    float peak = DSP::computePeakDbFS(fullScale);
    // Full scale (1.0) should be ~0 dBFS
    assert(peak > -0.1f && peak < 0.1f);
}

static void testComputePeakDbFS_withHalfAmplitude() {
    std::vector<float> halfScale = {0.5f, -0.5f, 0.25f};
    float peak = DSP::computePeakDbFS(halfScale);
    // 0.5 amplitude = -6.02 dBFS
    assert(peak > -6.5f && peak < -5.5f);
}

static void testComputePeakDbFS_emptyBuffer() {
    std::vector<float> empty;
    float peak = DSP::computePeakDbFS(empty);
    // Empty should return very negative dB
    assert(peak < -90.0f);
}

static void testComputePeakDbFS_singleSample() {
    std::vector<float> single = {0.25f};
    float peak = DSP::computePeakDbFS(single);
    // 0.25 amplitude = -12.04 dBFS
    assert(peak > -12.5f && peak < -11.5f);
}

static void testComputePeakDbFS_withSineWave() {
    auto sine = makeSine(440.0f, 48000, 48000, 2, 0.5f);
    float peak = DSP::computePeakDbFS(sine);
    // Peak of 0.5 amplitude sine = -6.02 dBFS
    assert(peak > -6.5f && peak < -5.5f);
}

// ============================================================================
// computeRMSDb tests
// ============================================================================

static void testComputeRMSDb_withSilence() {
    std::vector<float> silence = makeSilence(1000);
    float rms = DSP::computeRMSDb(silence);
    // Silence should return very negative dB
    assert(rms < -90.0f);
}

static void testComputeRMSDb_withDCSignal() {
    // DC signal of 0.5 has RMS of 0.5 = -6.02 dB
    std::vector<float> dc = makeConstant(1000, 0.5f);
    float rms = DSP::computeRMSDb(dc);
    assert(rms > -6.5f && rms < -5.5f);
}

static void testComputeRMSDb_withSineWave() {
    // RMS of sine wave = amplitude / sqrt(2)
    // For amplitude 1.0, RMS = 0.707 = -3.01 dB
    auto sine = makeSine(440.0f, 48000, 48000, 2, 1.0f);
    float rms = DSP::computeRMSDb(sine);
    assert(rms > -3.5f && rms < -2.5f);
}

static void testComputeRMSDb_emptyBuffer() {
    std::vector<float> empty;
    float rms = DSP::computeRMSDb(empty);
    assert(rms < -90.0f);
}

// ============================================================================
// normalizeToPeak tests
// ============================================================================

static void testNormalizeToPeak_toZeroDb() {
    auto samples = makeSine(440.0f, 48000, 48000, 2, 0.5f);
    float before = DSP::computePeakDbFS(samples);
    assert(before < -5.0f); // Should be around -6 dB
    
    DSP::normalizeToPeak(samples, 0.0f);
    float after = DSP::computePeakDbFS(samples);
    assert(approxEqual(after, 0.0f, 0.2f));
}

static void testNormalizeToPeak_toMinusThreeDb() {
    auto samples = makeSine(440.0f, 48000, 48000, 2, 0.5f);
    DSP::normalizeToPeak(samples, -3.0f);
    float after = DSP::computePeakDbFS(samples);
    assert(approxEqual(after, -3.0f, 0.2f));
}

static void testNormalizeToPeak_emptyBuffer() {
    std::vector<float> empty;
    DSP::normalizeToPeak(empty, -3.0f); // Should not crash
    assert(empty.empty());
}

static void testNormalizeToPeak_silentBuffer() {
    std::vector<float> silence = makeSilence(1000);
    DSP::normalizeToPeak(silence, 0.0f); // Should handle gracefully
    // Silence stays silence
    float peak = DSP::computePeakDbFS(silence);
    assert(peak < -90.0f);
}

static void testNormalizeToPeak_largeBuffer() {
    // Test parallel execution path (>10000 samples)
    auto samples = makeSine(440.0f, 48000, 48000, 2, 0.25f); // 96000 samples
    DSP::normalizeToPeak(samples, -1.0f);
    float after = DSP::computePeakDbFS(samples);
    assert(approxEqual(after, -1.0f, 0.2f));
}

// ============================================================================
// normalizeToRMS tests
// ============================================================================

static void testNormalizeToRMS_toMinusTwentyDb() {
    auto samples = makeSine(440.0f, 48000, 48000, 2, 0.5f);
    DSP::normalizeToRMS(samples, -20.0f);
    float afterRms = DSP::computeRMSDb(samples);
    assert(approxEqual(afterRms, -20.0f, 0.5f));
}

static void testNormalizeToRMS_emptyBuffer() {
    std::vector<float> empty;
    DSP::normalizeToRMS(empty, -20.0f); // Should not crash
    assert(empty.empty());
}

static void testNormalizeToRMS_largeBuffer() {
    // Test parallel execution path
    auto samples = makeSine(440.0f, 48000, 48000, 2, 0.25f);
    DSP::normalizeToRMS(samples, -14.0f);
    float afterRms = DSP::computeRMSDb(samples);
    assert(approxEqual(afterRms, -14.0f, 0.5f));
}

// ============================================================================
// compressor tests
// ============================================================================

static void testCompressor_noCompression() {
    // Signal below threshold should pass through unchanged
    auto samples = makeSine(440.0f, 48000, 4800, 2, 0.1f); // -20 dBFS peak
    auto original = samples;
    DSP::compressor(samples, -6.0f, 4.0f, 10.0f, 100.0f, 0.0f, 48000, 2);
    
    // Signal is well below -6dB threshold, should be mostly unchanged
    float originalPeak = DSP::computePeakDbFS(original);
    float compressedPeak = DSP::computePeakDbFS(samples);
    assert(approxEqual(originalPeak, compressedPeak, 1.0f));
}

static void testCompressor_withCompression() {
    // Signal above threshold should be compressed
    auto samples = makeSine(440.0f, 48000, 4800, 2, 1.0f); // 0 dBFS peak
    float beforePeak = DSP::computePeakDbFS(samples);
    
    // Threshold at -12dB, ratio 4:1, no makeup gain
    DSP::compressor(samples, -12.0f, 4.0f, 1.0f, 50.0f, 0.0f, 48000, 2);
    float afterPeak = DSP::computePeakDbFS(samples);
    
    // Should be reduced (compressed)
    assert(afterPeak < beforePeak);
}

static void testCompressor_withMakeupGain() {
    auto samples = makeSine(440.0f, 48000, 4800, 2, 0.5f);
    
    // Apply compression with 6dB makeup gain
    DSP::compressor(samples, -20.0f, 2.0f, 10.0f, 100.0f, 6.0f, 48000, 2);
    float afterPeak = DSP::computePeakDbFS(samples);
    
    // With makeup gain, output should be louder than compressed-only
    // Just verify it didn't crash and produced reasonable output
    assert(afterPeak > -20.0f && afterPeak < 6.0f);
}

static void testCompressor_emptyBuffer() {
    std::vector<float> empty;
    DSP::compressor(empty, -12.0f, 4.0f, 10.0f, 100.0f, 0.0f, 48000, 2);
    assert(empty.empty());
}

static void testCompressor_invalidSampleRate() {
    auto samples = makeSine(440.0f, 48000, 480, 2, 0.5f);
    auto original = samples;
    DSP::compressor(samples, -12.0f, 4.0f, 10.0f, 100.0f, 0.0f, 0, 2); // Invalid sample rate
    // Should return early without modifying
    assert(samples == original);
}

static void testCompressor_invalidChannels() {
    auto samples = makeSine(440.0f, 48000, 480, 2, 0.5f);
    auto original = samples;
    DSP::compressor(samples, -12.0f, 4.0f, 10.0f, 100.0f, 0.0f, 48000, 0); // Invalid channels
    // Should return early without modifying
    assert(samples == original);
}

static void testCompressor_monoSignal() {
    auto samples = makeSine(440.0f, 48000, 4800, 1, 1.0f);
    DSP::compressor(samples, -12.0f, 4.0f, 10.0f, 100.0f, 0.0f, 48000, 1);
    // Just verify it processes without crashing
    float afterPeak = DSP::computePeakDbFS(samples);
    assert(afterPeak < 0.0f);
}

// ============================================================================
// applyFadeIn tests
// ============================================================================

static void testApplyFadeIn_linearFade() {
    // Create a buffer of constant 1.0 samples
    std::vector<float> samples(100, 1.0f);
    
    // Apply linear fade in over 50 samples
    DSP::applyFadeIn(samples, 50, DSP::FadeType::Linear);
    
    // At sample 0: should be 0 (start of fade)
    assert(approxEqual(samples[0], 0.0f, 0.01f));
    
    // At sample 25: should be ~0.5 (middle of fade)
    assert(approxEqual(samples[25], 0.5f, 0.02f));
    
    // At sample 49: should be close to 1.0 (end of fade)
    assert(approxEqual(samples[49], 0.98f, 0.05f));
    
    // At sample 50+: should be unmodified (1.0)
    assert(approxEqual(samples[50], 1.0f, 0.01f));
    assert(approxEqual(samples[99], 1.0f, 0.01f));
}

static void testApplyFadeIn_exponentialFade() {
    std::vector<float> samples(100, 1.0f);
    
    DSP::applyFadeIn(samples, 50, DSP::FadeType::Exponential);
    
    // Exponential starts slower than linear
    assert(samples[0] < 0.01f);  // Should start at 0
    assert(samples[25] < 0.5f);   // Should be less than linear midpoint
    assert(samples[49] > 0.9f);   // Should approach 1.0 at end
    assert(approxEqual(samples[50], 1.0f, 0.01f));  // Unmodified after fade
}

static void testApplyFadeIn_sCurveFade() {
    std::vector<float> samples(100, 1.0f);
    
    DSP::applyFadeIn(samples, 50, DSP::FadeType::SCurve);
    
    // S-curve starts slow, accelerates in middle, slows at end
    assert(samples[0] < 0.01f);   // Should start at 0
    assert(samples[25] > 0.3f && samples[25] < 0.7f);  // Middle should be near 0.5
    assert(samples[49] > 0.95f);  // Should approach 1.0 at end
}

static void testApplyFadeIn_emptyBuffer() {
    std::vector<float> empty;
    DSP::applyFadeIn(empty, 50, DSP::FadeType::Linear);
    assert(empty.empty());  // Should not crash, remain empty
}

static void testApplyFadeIn_zeroLength() {
    std::vector<float> samples(100, 1.0f);
    auto original = samples;
    
    DSP::applyFadeIn(samples, 0, DSP::FadeType::Linear);
    
    // Zero-length fade should leave samples unchanged
    assert(samples == original);
}

static void testApplyFadeIn_fadeLongerThanBuffer() {
    std::vector<float> samples(10, 1.0f);
    
    // Fade length exceeds buffer - should apply fade to entire buffer
    DSP::applyFadeIn(samples, 100, DSP::FadeType::Linear);
    
    // All samples should be faded (partial fade)
    assert(samples[0] < 0.01f);
    assert(samples[9] < 1.0f);  // Last sample still fading
}

static void testApplyFadeIn_stereoBuffer() {
    // Stereo: L R L R L R ... (6 samples = 3 frames)
    std::vector<float> samples = {1.0f, 1.0f, 1.0f, 1.0f, 1.0f, 1.0f};
    
    // Fade over 3 frames (6 samples) - treats as interleaved stereo
    DSP::applyFadeIn(samples, 6, DSP::FadeType::Linear);
    
    // First sample pair should be near 0
    assert(samples[0] < 0.2f);
    assert(samples[1] < 0.2f);
}

// ============================================================================
// applyFadeOut tests
// ============================================================================

static void testApplyFadeOut_linearFade() {
    std::vector<float> samples(100, 1.0f);
    
    // Apply linear fade out over last 50 samples
    DSP::applyFadeOut(samples, 50, DSP::FadeType::Linear);
    
    // First 50 samples should be unmodified
    assert(approxEqual(samples[0], 1.0f, 0.01f));
    assert(approxEqual(samples[49], 1.0f, 0.01f));
    
    // At sample 50: start of fade, should be ~1.0
    assert(approxEqual(samples[50], 1.0f, 0.05f));
    
    // At sample 75: middle of fade, should be ~0.5
    assert(approxEqual(samples[75], 0.5f, 0.05f));
    
    // At sample 99: end of fade, should be near 0
    assert(samples[99] < 0.05f);
}

static void testApplyFadeOut_exponentialFade() {
    std::vector<float> samples(100, 1.0f);
    
    DSP::applyFadeOut(samples, 50, DSP::FadeType::Exponential);
    
    // Exponential fade drops faster initially
    assert(approxEqual(samples[49], 1.0f, 0.01f));  // Before fade
    assert(samples[75] > 0.5f);  // Exponential drops slower in middle
    assert(samples[99] < 0.1f);  // Near 0 at end
}

static void testApplyFadeOut_emptyBuffer() {
    std::vector<float> empty;
    DSP::applyFadeOut(empty, 50, DSP::FadeType::Linear);
    assert(empty.empty());
}

static void testApplyFadeOut_zeroLength() {
    std::vector<float> samples(100, 1.0f);
    auto original = samples;
    
    DSP::applyFadeOut(samples, 0, DSP::FadeType::Linear);
    
    assert(samples == original);
}

static void testApplyFadeOut_fadeLongerThanBuffer() {
    std::vector<float> samples(10, 1.0f);
    
    DSP::applyFadeOut(samples, 100, DSP::FadeType::Linear);
    
    // Entire buffer is fading
    assert(samples[0] > 0.8f);   // Start of fade (near 1.0)
    assert(samples[9] < 0.2f);   // End approaching 0
}

// ============================================================================
// Main test runner
// ============================================================================

int main() {
    // computePeakDbFS tests
    testComputePeakDbFS_withSilence();
    testComputePeakDbFS_withFullScale();
    testComputePeakDbFS_withHalfAmplitude();
    testComputePeakDbFS_emptyBuffer();
    testComputePeakDbFS_singleSample();
    testComputePeakDbFS_withSineWave();
    
    // computeRMSDb tests
    testComputeRMSDb_withSilence();
    testComputeRMSDb_withDCSignal();
    testComputeRMSDb_withSineWave();
    testComputeRMSDb_emptyBuffer();
    
    // normalizeToPeak tests
    testNormalizeToPeak_toZeroDb();
    testNormalizeToPeak_toMinusThreeDb();
    testNormalizeToPeak_emptyBuffer();
    testNormalizeToPeak_silentBuffer();
    testNormalizeToPeak_largeBuffer();
    
    // normalizeToRMS tests
    testNormalizeToRMS_toMinusTwentyDb();
    testNormalizeToRMS_emptyBuffer();
    testNormalizeToRMS_largeBuffer();
    
    // compressor tests
    testCompressor_noCompression();
    testCompressor_withCompression();
    testCompressor_withMakeupGain();
    testCompressor_emptyBuffer();
    testCompressor_invalidSampleRate();
    testCompressor_invalidChannels();
    testCompressor_monoSignal();
    
    // applyFadeIn tests
    testApplyFadeIn_linearFade();
    testApplyFadeIn_exponentialFade();
    testApplyFadeIn_sCurveFade();
    testApplyFadeIn_emptyBuffer();
    testApplyFadeIn_zeroLength();
    testApplyFadeIn_fadeLongerThanBuffer();
    testApplyFadeIn_stereoBuffer();
    
    // applyFadeOut tests
    testApplyFadeOut_linearFade();
    testApplyFadeOut_exponentialFade();
    testApplyFadeOut_emptyBuffer();
    testApplyFadeOut_zeroLength();
    testApplyFadeOut_fadeLongerThanBuffer();
    
    return 0;
}
