#include "AudioEngine.h"
#include <filesystem>

std::optional<AudioClip> AudioEngine::loadClip(const std::string& path) {
    const auto ext = std::filesystem::path(path).extension().string();
    std::optional<AudioClip> clip;
    if (ext == ".wav" || ext == ".WAV") {
        clip = wavCodec_.read(path);
    } else if (ext == ".mp3" || ext == ".MP3") {
        clip = mp3Codec_.read(path);
    }
    if (clip) {
        refreshMetrics(*clip);
    }
    return clip;
}

void AudioEngine::trim(AudioClip& clip, float startSec, float endSec) {
    const auto& data = clip.samples();
    int sr = clip.sampleRate();
    int ch = clip.channels();
    auto startIdx = static_cast<size_t>(std::max(0.0f, startSec) * sr * ch);
    auto endIdx = endSec <= 0 ? data.size() : static_cast<size_t>(endSec * sr * ch);
    startIdx = std::min(startIdx, data.size());
    endIdx = std::min(endIdx, data.size());
    if (startIdx >= endIdx) return;
    std::vector<float> trimmed(data.begin() + static_cast<std::ptrdiff_t>(startIdx), data.begin() + static_cast<std::ptrdiff_t>(endIdx));
    clip.setSamples(std::move(trimmed));
    refreshMetrics(clip);
}

void AudioEngine::normalizeToPeak(AudioClip& clip, float targetDbFS) {
    DSP::normalizeToPeak(clip.samplesMutable(), targetDbFS);
    refreshMetrics(clip);
}

void AudioEngine::normalizeToRms(AudioClip& clip, float targetDb) {
    DSP::normalizeToRMS(clip.samplesMutable(), targetDb);
    refreshMetrics(clip);
}

void AudioEngine::compress(AudioClip& clip, float thresholdDb, float ratio, float attackMs, float releaseMs, float makeupDb) {
    DSP::compressor(clip.samplesMutable(), thresholdDb, ratio, attackMs, releaseMs, makeupDb, clip.sampleRate(), clip.channels());
    refreshMetrics(clip);
}

bool AudioEngine::exportWav(const AudioClip& clip, const std::string& outFolder) {
    namespace fs = std::filesystem;
    fs::path folder(outFolder);
    fs::create_directories(folder);
    fs::path inPath(clip.filePath());
    auto outName = inPath.stem().string() + ".wav";
    fs::path outPath = folder / outName;
    return wavCodec_.write(outPath.string(), clip);
}

bool AudioEngine::exportMp3(
    const AudioClip& clip, 
    const std::string& outFolder,
    Mp3Encoder::BitrateMode bitrate,
    const Mp3Metadata& metadata
) {
    namespace fs = std::filesystem;
    fs::path folder(outFolder);
    fs::create_directories(folder);
    fs::path inPath(clip.filePath());
    auto outName = inPath.stem().string() + ".mp3";
    fs::path outPath = folder / outName;
    return mp3Encoder_.encode(clip, outPath.string(), bitrate, metadata);
}

void AudioEngine::updateClipMetrics(AudioClip& clip) {
    refreshMetrics(clip);
}

void AudioEngine::refreshMetrics(AudioClip& clip) {
    auto peak = DSP::computePeakDbFS(clip.samples());
    auto rms = DSP::computeRMSDb(clip.samples());
    clip.updateMetrics(peak, rms);
}



