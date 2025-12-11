#pragma once

#include <cstddef>
#include <tuple>

struct TrimAndFadeRanges {
    std::size_t visibleStartFrame;
    std::size_t visibleEndFrame;
    std::size_t fadeInStartFrame;
    std::size_t fadeInEndFrame;
    std::size_t fadeOutStartFrame;
    std::size_t fadeOutEndFrame;
};

TrimAndFadeRanges computeTrimAndFadeRanges(
    std::size_t clipFrameCount,
    std::size_t trimStartFrame,
    std::size_t trimEndFrame,
    bool showFullExtent,
    std::size_t fadeInLengthFrames,
    std::size_t fadeOutLengthFrames
);
