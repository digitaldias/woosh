#include "WaveformViewHelpers.h"

#include <algorithm>

TrimAndFadeRanges computeTrimAndFadeRanges(
    std::size_t clipFrameCount,
    std::size_t trimStartFrame,
    std::size_t trimEndFrame,
    bool showFullExtent,
    std::size_t fadeInLengthFrames,
    std::size_t fadeOutLengthFrames
) {
    TrimAndFadeRanges result{};

    if (clipFrameCount == 0) {
        return result;
    }

    std::size_t effectiveTrimEnd = trimEndFrame > 0 ? std::min(trimEndFrame, clipFrameCount) : clipFrameCount;
    effectiveTrimEnd = std::max(effectiveTrimEnd, static_cast<std::size_t>(1));

    std::size_t effectiveTrimStart = std::min(trimStartFrame, effectiveTrimEnd - 1);

    if (showFullExtent) {
        result.visibleStartFrame = 0;
        result.visibleEndFrame = clipFrameCount;
    } else {
        result.visibleStartFrame = effectiveTrimStart;
        result.visibleEndFrame = effectiveTrimEnd;
    }

    std::size_t activeStart = effectiveTrimStart;
    std::size_t activeEnd = effectiveTrimEnd;
    std::size_t activeLength = activeEnd > activeStart ? (activeEnd - activeStart) : 0;

    if (activeLength == 0) {
        return result;
    }

    std::size_t maxFadeEach = activeLength / 2;

    std::size_t clampedFadeIn = std::min(fadeInLengthFrames, maxFadeEach);
    std::size_t clampedFadeOut = std::min(fadeOutLengthFrames, maxFadeEach);

    result.fadeInStartFrame = activeStart;
    result.fadeInEndFrame = activeStart + clampedFadeIn;

    result.fadeOutEndFrame = activeEnd;
    result.fadeOutStartFrame = activeEnd > clampedFadeOut ? (activeEnd - clampedFadeOut) : activeStart;

    return result;
}
