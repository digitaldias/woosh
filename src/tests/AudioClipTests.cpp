/**
 * @file AudioClipTests.cpp
 * @brief Unit tests for AudioClip class.
 */

#include <cassert>
#include <cmath>
#include <vector>
#include <string>
#include "audio/AudioClip.h"

// ============================================================================
// Helper functions
// ============================================================================

static std::vector<float> makeStereoSamples(size_t frames, float value = 0.5f) {
    std::vector<float> data(frames * 2);
    for (size_t i = 0; i < data.size(); ++i) {
        data[i] = value;
    }
    return data;
}

static std::vector<float> makeMonoSamples(size_t frames, float value = 0.5f) {
    return std::vector<float>(frames, value);
}

static bool approxEqual(double a, double b, double tolerance = 0.001) {
    return std::abs(a - b) < tolerance;
}

// ============================================================================
// Constructor tests
// ============================================================================

static void testConstructor_defaultConstructor() {
    AudioClip clip;
    assert(clip.filePath().empty());
    assert(clip.displayName().empty());
    assert(clip.sampleRate() == 44100); // Default
    assert(clip.channels() == 2);       // Default
    assert(clip.samples().empty());
    assert(clip.frameCount() == 0);
    assert(!clip.hasOriginal());
    assert(!clip.isModified());
}

static void testConstructor_withParameters() {
    auto samples = makeStereoSamples(1000);
    AudioClip clip("test/audio/file.wav", 48000, 2, samples);
    
    assert(clip.filePath() == "test/audio/file.wav");
    assert(clip.sampleRate() == 48000);
    assert(clip.channels() == 2);
    assert(clip.samples().size() == 2000);
    assert(!clip.hasOriginal());
    assert(!clip.isModified());
}

static void testConstructor_monoFile() {
    auto samples = makeMonoSamples(500);
    AudioClip clip("mono.wav", 44100, 1, samples);
    
    assert(clip.channels() == 1);
    assert(clip.samples().size() == 500);
    assert(clip.frameCount() == 500);
}

// ============================================================================
// Duration and frame count tests
// ============================================================================

static void testDurationSeconds_stereo() {
    // 48000 frames at 48000 Hz = 1 second
    auto samples = makeStereoSamples(48000);
    AudioClip clip("test.wav", 48000, 2, samples);
    
    assert(approxEqual(clip.durationSeconds(), 1.0));
}

static void testDurationSeconds_mono() {
    // 44100 frames at 44100 Hz = 1 second
    auto samples = makeMonoSamples(44100);
    AudioClip clip("test.wav", 44100, 1, samples);
    
    assert(approxEqual(clip.durationSeconds(), 1.0));
}

static void testDurationSeconds_halfSecond() {
    // 24000 frames at 48000 Hz = 0.5 seconds
    auto samples = makeStereoSamples(24000);
    AudioClip clip("test.wav", 48000, 2, samples);
    
    assert(approxEqual(clip.durationSeconds(), 0.5));
}

static void testDurationSeconds_empty() {
    AudioClip clip;
    assert(clip.durationSeconds() == 0.0);
}

static void testFrameCount_stereo() {
    auto samples = makeStereoSamples(1000);
    AudioClip clip("test.wav", 48000, 2, samples);
    
    // 2000 samples / 2 channels = 1000 frames
    assert(clip.frameCount() == 1000);
}

static void testFrameCount_mono() {
    auto samples = makeMonoSamples(500);
    AudioClip clip("test.wav", 44100, 1, samples);
    
    assert(clip.frameCount() == 500);
}

static void testFrameCount_empty() {
    AudioClip clip;
    assert(clip.frameCount() == 0);
}

// ============================================================================
// setSamples tests
// ============================================================================

static void testSetSamples_replaceSamples() {
    auto original = makeStereoSamples(1000, 0.5f);
    AudioClip clip("test.wav", 48000, 2, original);
    
    auto newSamples = makeStereoSamples(500, 0.25f);
    clip.setSamples(std::move(newSamples));
    
    assert(clip.samples().size() == 1000); // 500 frames * 2 channels
    assert(clip.frameCount() == 500);
}

static void testSetSamples_clearSamples() {
    auto original = makeStereoSamples(1000);
    AudioClip clip("test.wav", 48000, 2, original);
    
    clip.setSamples(std::vector<float>());
    
    assert(clip.samples().empty());
    assert(clip.frameCount() == 0);
}

// ============================================================================
// setFilePath tests
// ============================================================================

static void testSetFilePath_updatesPath() {
    AudioClip clip("original.wav", 48000, 2, makeStereoSamples(100));
    clip.setFilePath("/new/path/audio.wav");
    
    assert(clip.filePath() == "/new/path/audio.wav");
}

static void testSetFilePath_extractsDisplayName() {
    AudioClip clip;
    clip.setFilePath("/path/to/my_audio_file.wav");
    
    // Display name should be the filename without path
    assert(clip.displayName() == "my_audio_file.wav");
}

// ============================================================================
// Metrics tests
// ============================================================================

static void testUpdateMetrics_storesValues() {
    AudioClip clip("test.wav", 48000, 2, makeStereoSamples(100));
    clip.updateMetrics(-6.0f, -12.0f);
    
    assert(approxEqual(clip.peakDb(), -6.0f, 0.01f));
    assert(approxEqual(clip.rmsDb(), -12.0f, 0.01f));
}

// ============================================================================
// Undo support tests (saveOriginal / restoreOriginal)
// ============================================================================

static void testSaveOriginal_storesCurrentState() {
    auto samples = makeStereoSamples(1000, 0.5f);
    AudioClip clip("test.wav", 48000, 2, samples);
    clip.updateMetrics(-6.0f, -12.0f);
    
    assert(!clip.hasOriginal());
    
    clip.saveOriginal();
    
    assert(clip.hasOriginal());
}

static void testRestoreOriginal_restoresSamples() {
    auto originalData = makeStereoSamples(1000, 0.5f);
    AudioClip clip("test.wav", 48000, 2, originalData);
    clip.updateMetrics(-6.0f, -12.0f);
    clip.saveOriginal();
    
    // Modify samples
    auto modifiedData = makeStereoSamples(500, 0.25f);
    clip.setSamples(std::move(modifiedData));
    clip.updateMetrics(-12.0f, -18.0f);
    clip.setModified(true);
    
    assert(clip.frameCount() == 500);
    assert(clip.isModified());
    
    // Restore original
    clip.restoreOriginal();
    
    assert(clip.frameCount() == 1000);
    assert(clip.samples().size() == 2000);
    assert(!clip.isModified());
    // Metrics should also be restored
    assert(approxEqual(clip.peakDb(), -6.0f, 0.01f));
    assert(approxEqual(clip.rmsDb(), -12.0f, 0.01f));
}

static void testRestoreOriginal_withoutSave() {
    auto samples = makeStereoSamples(1000);
    AudioClip clip("test.wav", 48000, 2, samples);
    
    // Restore without having saved original
    clip.restoreOriginal(); // Should not crash
    
    // Samples should be unchanged
    assert(clip.samples().size() == 2000);
}

static void testHasOriginal_falseByDefault() {
    AudioClip clip("test.wav", 48000, 2, makeStereoSamples(100));
    assert(!clip.hasOriginal());
}

static void testHasOriginal_trueAfterSave() {
    AudioClip clip("test.wav", 48000, 2, makeStereoSamples(100));
    clip.saveOriginal();
    assert(clip.hasOriginal());
}

// ============================================================================
// Modified flag tests
// ============================================================================

static void testIsModified_falseByDefault() {
    AudioClip clip("test.wav", 48000, 2, makeStereoSamples(100));
    assert(!clip.isModified());
}

static void testSetModified_setsFlag() {
    AudioClip clip("test.wav", 48000, 2, makeStereoSamples(100));
    
    clip.setModified(true);
    assert(clip.isModified());
    
    clip.setModified(false);
    assert(!clip.isModified());
}

static void testRestoreOriginal_clearsModifiedFlag() {
    AudioClip clip("test.wav", 48000, 2, makeStereoSamples(100));
    clip.saveOriginal();
    clip.setModified(true);
    
    clip.restoreOriginal();
    
    assert(!clip.isModified());
}

// ============================================================================
// Edge cases
// ============================================================================

static void testClip_veryLargeSamples() {
    // 10 seconds of stereo at 48kHz = 960,000 samples
    auto samples = makeStereoSamples(480000);
    AudioClip clip("large.wav", 48000, 2, samples);
    
    assert(clip.frameCount() == 480000);
    assert(approxEqual(clip.durationSeconds(), 10.0));
}

static void testClip_highSampleRate() {
    auto samples = makeStereoSamples(96000);
    AudioClip clip("hires.wav", 96000, 2, samples);
    
    assert(clip.sampleRate() == 96000);
    assert(approxEqual(clip.durationSeconds(), 1.0));
}

static void testClip_multiChannel() {
    // 5.1 surround = 6 channels
    std::vector<float> samples(48000 * 6, 0.5f);
    AudioClip clip("surround.wav", 48000, 6, samples);
    
    assert(clip.channels() == 6);
    assert(clip.frameCount() == 48000);
    assert(approxEqual(clip.durationSeconds(), 1.0));
}

// ============================================================================
// Main test runner
// ============================================================================

int main() {
    // Constructor tests
    testConstructor_defaultConstructor();
    testConstructor_withParameters();
    testConstructor_monoFile();
    
    // Duration and frame count tests
    testDurationSeconds_stereo();
    testDurationSeconds_mono();
    testDurationSeconds_halfSecond();
    testDurationSeconds_empty();
    testFrameCount_stereo();
    testFrameCount_mono();
    testFrameCount_empty();
    
    // setSamples tests
    testSetSamples_replaceSamples();
    testSetSamples_clearSamples();
    
    // setFilePath tests
    testSetFilePath_updatesPath();
    testSetFilePath_extractsDisplayName();
    
    // Metrics tests
    testUpdateMetrics_storesValues();
    
    // Undo support tests
    testSaveOriginal_storesCurrentState();
    testRestoreOriginal_restoresSamples();
    testRestoreOriginal_withoutSave();
    testHasOriginal_falseByDefault();
    testHasOriginal_trueAfterSave();
    
    // Modified flag tests
    testIsModified_falseByDefault();
    testSetModified_setsFlag();
    testRestoreOriginal_clearsModifiedFlag();
    
    // Edge cases
    testClip_veryLargeSamples();
    testClip_highSampleRate();
    testClip_multiChannel();
    
    return 0;
}
