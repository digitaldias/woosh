#pragma once

#include <optional>
#include <string>
#include <vector>
#include "AudioClip.h"
#include "Formats/WavCodec.h"
#include "Formats/Mp3Codec.h"
#include "utils/DSP.h"

class AudioEngine {
public:
    AudioEngine() = default;

    [[nodiscard]] std::optional<AudioClip> loadClip(const std::string& path);
    void trim(AudioClip& clip, float startSec, float endSec);
    void normalizeToPeak(AudioClip& clip, float targetDbFS);
    void normalizeToRms(AudioClip& clip, float targetDb);
    void compress(AudioClip& clip, float thresholdDb, float ratio, float attackMs, float releaseMs, float makeupDb);
    [[nodiscard]] bool exportWav(const AudioClip& clip, const std::string& outFolder);

    /** @brief Recalculate peak/RMS metrics for a clip (call after manual sample edits). */
    void updateClipMetrics(AudioClip& clip);

private:
    void refreshMetrics(AudioClip& clip);
    WavCodec wavCodec_;
    Mp3Codec mp3Codec_;
};



