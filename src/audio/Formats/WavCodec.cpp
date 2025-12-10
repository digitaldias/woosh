#include "WavCodec.h"
#include <sndfile.hh>
#include <vector>

std::optional<AudioClip> WavCodec::read(const std::string& path) {
    SndfileHandle handle(path);
    if (!handle || handle.error()) {
        return std::nullopt;
    }
    auto frames = static_cast<size_t>(handle.frames());
    auto channels = handle.channels();
    auto sampleRate = handle.samplerate();
    std::vector<float> data(frames * static_cast<size_t>(channels));
    auto read = handle.readf(data.data(), frames);
    data.resize(static_cast<size_t>(read) * static_cast<size_t>(channels));
    return AudioClip(path, sampleRate, channels, std::move(data));
}

bool WavCodec::write(const std::string& path, const AudioClip& clip) {
    SndfileHandle handle(path, SFM_WRITE, SF_FORMAT_WAV | SF_FORMAT_PCM_16, clip.channels(), clip.sampleRate());
    if (!handle || handle.error()) return false;
    auto frames = static_cast<sf_count_t>(clip.samples().size() / static_cast<size_t>(clip.channels()));
    auto written = handle.writef(clip.samples().data(), frames);
    return written == frames;
}


