#include <cassert>
#include "ui/WaveformViewHelpers.h"

static void testComputeTrimAndFadeRanges_noTrim_fullExtent() {
    TrimAndFadeRanges r = computeTrimAndFadeRanges(100, 0, 0, true, 0, 0);
    assert(r.visibleStartFrame == 0);
    assert(r.visibleEndFrame == 100);
}

static void testComputeTrimAndFadeRanges_trimmed_clipView() {
    TrimAndFadeRanges r = computeTrimAndFadeRanges(100, 10, 90, false, 0, 0);
    assert(r.visibleStartFrame == 10);
    assert(r.visibleEndFrame == 90);
}

static void testComputeTrimAndFadeRanges_fades_clamped() {
    TrimAndFadeRanges r = computeTrimAndFadeRanges(100, 10, 90, false, 1000, 1000);
    // Active region is 80 frames (10..90), so max fade each is 40
    assert(r.fadeInStartFrame == 10);
    assert(r.fadeInEndFrame == 50);
    assert(r.fadeOutStartFrame == 50);
    assert(r.fadeOutEndFrame == 90);
}

static void testComputeTrimAndFadeRanges_emptyClip() {
    TrimAndFadeRanges r = computeTrimAndFadeRanges(0, 0, 0, true, 10, 10);
    assert(r.visibleStartFrame == 0);
    assert(r.visibleEndFrame == 0);
}

int main() {
    testComputeTrimAndFadeRanges_noTrim_fullExtent();
    testComputeTrimAndFadeRanges_trimmed_clipView();
    testComputeTrimAndFadeRanges_fades_clamped();
    testComputeTrimAndFadeRanges_emptyClip();
    return 0;
}
