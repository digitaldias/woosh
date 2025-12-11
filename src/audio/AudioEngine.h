#pragma once

#include <optional>
#include <string>
#include <vector>
#include "AudioClip.h"
#include "Formats/WavCodec.h"
#include "Formats/Mp3Codec.h"
#include "Formats/Mp3Encoder.h"
#include "utils/DSP.h"

class AudioEngine {
public:
    AudioEngine() = default;

    [[nodiscard]] std::optional<AudioClip> loadClip(const std::string& path);
    void trim(AudioClip& clip, float startSec, float endSec);
    void normalizeToPeak(AudioClip& clip, float targetDbFS);
    void normalizeToRms(AudioClip& clip, float targetDb);
    void compress(AudioClip& clip, float thresholdDb, float ratio, float attackMs, float releaseMs, float makeupDb);
    [[nodiscard]] bool exportWav(const AudioClip& clip, const std::string& outFolder, int fadeInFrames = 0, int fadeOutFrames = 0);
    
    /**
     * @brief Export an audio clip to MP3 format.
     * 
     * @param clip The audio clip to export.
     * @param outFolder The output folder path.
     * @param bitrate The bitrate mode (128, 160, 192 kbps or VBR).
     * @param metadata ID3 tag metadata.
     * @param fadeInFrames Fade in length in frames (0 = no fade).
     * @param fadeOutFrames Fade out length in frames (0 = no fade).
     * @return true if export succeeded, false otherwise.
     */
    [[nodiscard]] bool exportMp3(
        const AudioClip& clip, 
        const std::string& outFolder,
        Mp3Encoder::BitrateMode bitrate = Mp3Encoder::BitrateMode::CBR_160,
        const Mp3Metadata& metadata = {},
        int fadeInFrames = 0,
        int fadeOutFrames = 0
    );

    /** @brief Recalculate peak/RMS metrics for a clip (call after manual sample edits). */
    void updateClipMetrics(AudioClip& clip);

private:
    void refreshMetrics(AudioClip& clip);
    WavCodec wavCodec_;
    Mp3Codec mp3Codec_;
    Mp3Encoder mp3Encoder_;
};



