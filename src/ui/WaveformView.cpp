/**
 * @file WaveformView.cpp
 * @brief Implementation of WaveformView widget with stereo display.
 */

#include "WaveformView.h"
#include "audio/AudioClip.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QLinearGradient>
#include <QPainterPath>
#include <QtMath>
#include <algorithm>
#include <cmath>
#include <execution>
#include <numeric>

// ============================================================================
// Construction
// ============================================================================

WaveformView::WaveformView(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(120);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);
}

WaveformView::~WaveformView() = default;

// ============================================================================
// Clip management
// ============================================================================

void WaveformView::setClip(AudioClip* clip) {
    clip_ = clip;
    cacheValid_ = false;
    scrollOffsetFrames_ = 0;
    clearTrim();
    clearPlayhead();

    if (clip_) {
        zoomToFit();
    }
    update();
}

// ============================================================================
// Zoom controls
// ============================================================================

void WaveformView::zoomIn() {
    zoomAtPoint(width() / 2, 0.5);
}

void WaveformView::zoomOut() {
    zoomAtPoint(width() / 2, 2.0);
}

void WaveformView::zoomAtPoint(int pixelX, double factor) {
    if (!clip_) return;

    // Get frame under cursor before zoom
    int frameUnderCursor = xToFrame(pixelX);

    // Calculate new samples per pixel
    double newSpp = samplesPerPixel_ * factor;

    // Clamp to valid range
    double minSpp = 1.0;  // Max zoom: 1 sample per pixel
    double maxSpp = static_cast<double>(clip_->frameCount()) / width();
    newSpp = std::clamp(newSpp, minSpp, maxSpp);

    if (newSpp == samplesPerPixel_) return;

    samplesPerPixel_ = newSpp;

    // Calculate new scroll offset to keep frameUnderCursor at pixelX
    int newOffset = frameUnderCursor - static_cast<int>(pixelX * samplesPerPixel_);

    // Clamp scroll offset
    int maxScroll = std::max(0, static_cast<int>(clip_->frameCount() - width() * samplesPerPixel_));
    scrollOffsetFrames_ = std::clamp(newOffset, 0, maxScroll);

    cacheValid_ = false;
    update();
}

void WaveformView::zoomToFit() {
    if (!clip_ || width() <= 0) return;

    samplesPerPixel_ = static_cast<double>(clip_->frameCount()) / width();
    if (samplesPerPixel_ < 1.0) samplesPerPixel_ = 1.0;

    scrollOffsetFrames_ = 0;
    cacheValid_ = false;
    update();
}

// ============================================================================
// Trim controls
// ============================================================================

void WaveformView::setTrimStartFrame(int frame) {
    if (!clip_) return;
    trimStartFrame_ = std::clamp(frame, 0, static_cast<int>(clip_->frameCount()) - 1);
    if (trimEndFrame_ > 0 && trimStartFrame_ >= trimEndFrame_) {
        trimStartFrame_ = trimEndFrame_ - 1;
    }
    Q_EMIT trimChanged(trimStartFrame_, trimEndFrame_);
    update();
}

void WaveformView::setTrimEndFrame(int frame) {
    if (!clip_) return;
    int maxFrame = static_cast<int>(clip_->frameCount());
    trimEndFrame_ = std::clamp(frame, 0, maxFrame);
    if (trimEndFrame_ > 0 && trimEndFrame_ <= trimStartFrame_) {
        trimEndFrame_ = trimStartFrame_ + 1;
    }
    Q_EMIT trimChanged(trimStartFrame_, trimEndFrame_);
    update();
}

void WaveformView::clearTrim() {
    trimStartFrame_ = 0;
    trimEndFrame_ = 0;
    Q_EMIT trimChanged(0, 0);
    update();
}

bool WaveformView::hasTrimRegion() const {
    return trimStartFrame_ > 0 || trimEndFrame_ > 0;
}

void WaveformView::setShowFullExtent(bool show) {
    showFullExtent_ = show;
    cacheValid_ = false;
    update();
}

void WaveformView::setFadeInLengthFrames(int frames) {
    int newFrames = std::max(0, frames);
    if (newFrames != fadeInLengthFrames_) {
        fadeInLengthFrames_ = newFrames;
        Q_EMIT fadeChanged(fadeInLengthFrames_, fadeOutLengthFrames_);
        update();
    }
}

void WaveformView::setFadeOutLengthFrames(int frames) {
    int newFrames = std::max(0, frames);
    if (newFrames != fadeOutLengthFrames_) {
        fadeOutLengthFrames_ = newFrames;
        Q_EMIT fadeChanged(fadeInLengthFrames_, fadeOutLengthFrames_);
        update();
    }
}

void WaveformView::setEditMode(bool isFadeMode) {
    isFadeMode_ = isFadeMode;
    update();
}

// ============================================================================
// Playhead
// ============================================================================

void WaveformView::setPlayheadFrame(int frame) {
    playheadFrame_ = frame;
    update();
}

void WaveformView::clearPlayhead() {
    playheadFrame_ = -1;
    update();
}

// ============================================================================
// Paint event
// ============================================================================

void WaveformView::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event)
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing, false);

    QRect waveformRect = rect();
    waveformRect.setBottom(rect().bottom() - kRulerHeight);

    QRect rulerRect = rect();
    rulerRect.setTop(rect().bottom() - kRulerHeight);

    drawBackground(painter, waveformRect);

    if (!clip_ || clip_->samples().empty()) {
        painter.setPen(QColor(80, 80, 90));
        QFont f = painter.font();
        f.setPointSize(11);
        painter.setFont(f);
        painter.drawText(waveformRect, Qt::AlignCenter, "Select a clip to preview");
        drawTimeRuler(painter, rulerRect);
        return;
    }

    if (!cacheValid_) {
        computeWaveformCache();
    }

    drawWaveform(painter, waveformRect);
    drawTrimRegion(painter, waveformRect);
    drawFadeRegions(painter, waveformRect);
    drawPlayhead(painter, waveformRect);
    drawTimeRuler(painter, rulerRect);
}

void WaveformView::resizeEvent(QResizeEvent* event) {
    Q_UNUSED(event)
    cacheValid_ = false;
}

// ============================================================================
// Background rendering
// ============================================================================

void WaveformView::drawBackground(QPainter& painter, const QRect& rect) {
    // Solid dark background like reference
    painter.fillRect(rect, QColor(28, 30, 38));

    int channels = clip_ ? clip_->channels() : 1;
    if (channels >= 2) {
        // Stereo: draw channel separator
        int halfHeight = rect.height() / 2;

        // Channel separator line
        painter.setPen(QColor(55, 60, 75));
        painter.drawLine(rect.left(), rect.top() + halfHeight, rect.right(), rect.top() + halfHeight);
    }
}

// ============================================================================
// Waveform rendering
// ============================================================================

void WaveformView::computeWaveformCache() {
    channelCache_.clear();
    if (!clip_ || clip_->samples().empty()) {
        cacheValid_ = true;
        return;
    }

    const auto& samples = clip_->samples();
    int channels = clip_->channels();
    size_t frameCount = clip_->frameCount();
    int widgetWidth = width();

    channelCache_.resize(channels);
    for (int ch = 0; ch < channels; ++ch) {
        channelCache_[ch].resize(widgetWidth);
    }

    // Threshold for parallel execution (overhead not worth it for small displays)
    constexpr int kParallelThreshold = 200;

    if (widgetWidth >= kParallelThreshold) {
        // Parallel waveform computation - each column is independent
        std::vector<int> columnIndices(widgetWidth);
        std::iota(columnIndices.begin(), columnIndices.end(), 0);

        std::for_each(std::execution::par_unseq, columnIndices.begin(), columnIndices.end(),
            [this, &samples, channels, frameCount, widgetWidth](int x) {
                int startFrame = scrollOffsetFrames_ + static_cast<int>(x * samplesPerPixel_);
                int endFrame = scrollOffsetFrames_ + static_cast<int>((x + 1) * samplesPerPixel_);

                startFrame = std::clamp(startFrame, 0, static_cast<int>(frameCount) - 1);
                endFrame = std::clamp(endFrame, startFrame + 1, static_cast<int>(frameCount));

                for (int ch = 0; ch < channels; ++ch) {
                    float minVal = 0.0f;
                    float maxVal = 0.0f;

                    for (int f = startFrame; f < endFrame; ++f) {
                        size_t idx = static_cast<size_t>(f * channels + ch);
                        if (idx < samples.size()) {
                            float val = samples[idx];
                            minVal = std::min(minVal, val);
                            maxVal = std::max(maxVal, val);
                        }
                    }

                    channelCache_[ch][x] = {minVal, maxVal};
                }
            });
    } else {
        // Sequential for small widths (parallel overhead not worth it)
        for (int x = 0; x < widgetWidth; ++x) {
            int startFrame = scrollOffsetFrames_ + static_cast<int>(x * samplesPerPixel_);
            int endFrame = scrollOffsetFrames_ + static_cast<int>((x + 1) * samplesPerPixel_);

            startFrame = std::clamp(startFrame, 0, static_cast<int>(frameCount) - 1);
            endFrame = std::clamp(endFrame, startFrame + 1, static_cast<int>(frameCount));

            for (int ch = 0; ch < channels; ++ch) {
                float minVal = 0.0f;
                float maxVal = 0.0f;

                for (int f = startFrame; f < endFrame; ++f) {
                    size_t idx = static_cast<size_t>(f * channels + ch);
                    if (idx < samples.size()) {
                        float val = samples[idx];
                        minVal = std::min(minVal, val);
                        maxVal = std::max(maxVal, val);
                    }
                }

                channelCache_[ch][x] = {minVal, maxVal};
            }
        }
    }

    cacheValid_ = true;
}

void WaveformView::drawWaveform(QPainter& painter, const QRect& rect) {
    if (channelCache_.empty()) return;

    int channels = static_cast<int>(channelCache_.size());

    if (channels >= 2) {
        // Stereo: top half = left, bottom half = right
        int halfHeight = (rect.height() - kChannelGap) / 2;

        QRect leftRect(rect.left(), rect.top(), rect.width(), halfHeight);
        QRect rightRect(rect.left(), rect.top() + halfHeight + kChannelGap, rect.width(), halfHeight);

        drawWaveformChannel(painter, leftRect, 0, false);
        drawWaveformChannel(painter, rightRect, 1, true);
    } else {
        // Mono: use full height
        drawWaveformChannel(painter, rect, 0, false);
    }
}

void WaveformView::drawWaveformChannel(QPainter& painter, const QRect& rect, int channel, bool flipY) {
    if (channel >= static_cast<int>(channelCache_.size())) return;
    const auto& cache = channelCache_[channel];
    if (cache.empty()) return;

    int centerY = rect.center().y();
    int halfHeight = rect.height() / 2 - 2;

    // Waveform colors - purple/blue like the reference
    QColor waveColor = channel == 0 ? QColor(130, 140, 220) : QColor(130, 140, 220);

    painter.setPen(waveColor);

    // Draw vertical lines for each pixel column (traditional waveform style)
    for (int x = 0; x < static_cast<int>(cache.size()) && x < rect.width(); ++x) {
        const auto& col = cache[x];

        // Get min/max values, flip if needed (for mirrored bottom channel)
        float maxV = flipY ? -col.minVal : col.maxVal;
        float minV = flipY ? -col.maxVal : col.minVal;

        // Map to pixel coordinates
        int yTop = centerY - static_cast<int>(maxV * halfHeight);
        int yBot = centerY - static_cast<int>(minV * halfHeight);

        // Ensure we draw at least 1 pixel
        if (yTop == yBot) {
            yBot = yTop + 1;
        }

        // Draw vertical line from min to max
        int xPos = rect.left() + x;
        painter.drawLine(xPos, yTop, xPos, yBot);
    }

    // Draw center line (zero crossing)
    painter.setPen(QColor(70, 75, 85));
    painter.drawLine(rect.left(), centerY, rect.right(), centerY);
}

void WaveformView::drawTrimRegion(QPainter& painter, const QRect& rect) {
    if (!clip_) return;

    int maxFrame = static_cast<int>(clip_->frameCount());
    int effectiveEndFrame = (trimEndFrame_ > 0) ? trimEndFrame_ : maxFrame;

    // Only show trim overlays if showing full extent
    if (showFullExtent_) {
        // Semi-transparent overlay for trimmed regions
        QColor trimmedColor(0, 0, 0, 140);

        // Left trimmed region
        if (trimStartFrame_ > 0) {
            int xEnd = frameToX(trimStartFrame_);
            if (xEnd > rect.left()) {
                painter.fillRect(rect.left(), rect.top(), xEnd - rect.left(), rect.height(), trimmedColor);
            }
        }

        // Right trimmed region
        if (trimEndFrame_ > 0 && trimEndFrame_ < maxFrame) {
            int xStart = frameToX(trimEndFrame_);
            if (xStart < rect.right()) {
                painter.fillRect(xStart, rect.top(), rect.right() - xStart, rect.height(), trimmedColor);
            }
        }
    }

    // Choose colors based on edit mode
    QColor handleColor = isFadeMode_ ? QColor(0, 200, 255) : QColor(255, 180, 60);  // Cyan vs Yellow
    QColor handleLineColor = isFadeMode_ ? QColor(80, 220, 255) : QColor(255, 200, 80);

    // Start handle
    int xStart = frameToX(trimStartFrame_);
    if (xStart >= rect.left() - kHandleWidth && xStart <= rect.right()) {
        // Handle line
        painter.setPen(QPen(handleLineColor, 2));
        painter.drawLine(xStart, rect.top(), xStart, rect.bottom());

        // Triangle grip
        QPolygon startHandle;
        startHandle << QPoint(xStart, rect.top())
                    << QPoint(xStart + kHandleWidth, rect.top())
                    << QPoint(xStart, rect.top() + kHandleWidth * 2);
        painter.setPen(Qt::NoPen);
        painter.setBrush(handleColor);
        painter.drawPolygon(startHandle);
    }

    // End handle
    int xEnd = frameToX(effectiveEndFrame);
    if (xEnd >= rect.left() && xEnd <= rect.right() + kHandleWidth) {
        painter.setPen(QPen(handleLineColor, 2));
        painter.drawLine(xEnd, rect.top(), xEnd, rect.bottom());

        QPolygon endHandle;
        endHandle << QPoint(xEnd, rect.top())
                  << QPoint(xEnd - kHandleWidth, rect.top())
                  << QPoint(xEnd, rect.top() + kHandleWidth * 2);
        painter.setPen(Qt::NoPen);
        painter.setBrush(handleColor);
        painter.drawPolygon(endHandle);
    }
}

void WaveformView::drawFadeRegions(QPainter& painter, const QRect& rect) {
    if (!clip_ || !isFadeMode_ || (fadeInLengthFrames_ == 0 && fadeOutLengthFrames_ == 0)) return;

    int maxFrame = static_cast<int>(clip_->frameCount());
    int effectiveEndFrame = (trimEndFrame_ > 0) ? trimEndFrame_ : maxFrame;
    int activeLength = effectiveEndFrame - trimStartFrame_;
    
    if (activeLength <= 0) return;

    // Clamp fades to half the active region each
    int maxFadeEach = activeLength / 2;
    int clampedFadeIn = std::min(fadeInLengthFrames_, maxFadeEach);
    int clampedFadeOut = std::min(fadeOutLengthFrames_, maxFadeEach);

    QColor fadeLineColor(0, 200, 255);  // Cyan
    QColor fadeFillColor(0, 200, 255, 30);

    painter.setRenderHint(QPainter::Antialiasing, true);

    // Fade in curve (S-curve from bottom to top)
    if (clampedFadeIn > 0) {
        int xStart = frameToX(trimStartFrame_);
        int xEnd = frameToX(trimStartFrame_ + clampedFadeIn);
        
        if (xEnd > rect.left() && xStart < rect.right()) {
            int centerY = rect.center().y();
            int halfHeight = rect.height() / 2;

            // Draw smooth S-curve fade line
            QPainterPath fadePath;
            fadePath.moveTo(xStart, rect.bottom());
            
            int steps = std::min(100, xEnd - xStart);
            for (int i = 0; i <= steps; ++i) {
                float t = static_cast<float>(i) / steps;
                // S-curve: smoothstep function
                float gain = t * t * (3.0f - 2.0f * t);
                int x = xStart + static_cast<int>((xEnd - xStart) * t);
                int y = rect.bottom() - static_cast<int>(halfHeight * 2 * gain);
                fadePath.lineTo(x, y);
            }

            // Draw filled area under curve
            QPainterPath fillPath = fadePath;
            fillPath.lineTo(xEnd, rect.bottom());
            fillPath.lineTo(xStart, rect.bottom());
            painter.fillPath(fillPath, fadeFillColor);

            // Draw curve line
            painter.setPen(QPen(fadeLineColor, 2));
            painter.drawPath(fadePath);
        }
    }

    // Fade out curve (S-curve from top to bottom)
    if (clampedFadeOut > 0) {
        int xStart = frameToX(effectiveEndFrame - clampedFadeOut);
        int xEnd = frameToX(effectiveEndFrame);
        
        if (xStart < rect.right() && xEnd > rect.left()) {
            int centerY = rect.center().y();
            int halfHeight = rect.height() / 2;

            // Draw smooth S-curve fade line
            QPainterPath fadePath;
            fadePath.moveTo(xStart, rect.top());
            
            int steps = std::min(100, xEnd - xStart);
            for (int i = 0; i <= steps; ++i) {
                float t = static_cast<float>(i) / steps;
                // S-curve: smoothstep function (inverted for fade-out)
                float gain = 1.0f - (t * t * (3.0f - 2.0f * t));
                int x = xStart + static_cast<int>((xEnd - xStart) * t);
                int y = rect.top() + static_cast<int>(halfHeight * 2 * (1.0f - gain));
                fadePath.lineTo(x, y);
            }

            // Draw filled area under curve
            QPainterPath fillPath = fadePath;
            fillPath.lineTo(xEnd, rect.bottom());
            fillPath.lineTo(xStart, rect.bottom());
            painter.fillPath(fillPath, fadeFillColor);

            // Draw curve line
            painter.setPen(QPen(fadeLineColor, 2));
            painter.drawPath(fadePath);
        }
    }

    painter.setRenderHint(QPainter::Antialiasing, false);
}

void WaveformView::drawPlayhead(QPainter& painter, const QRect& rect) {
    if (playheadFrame_ < 0 || !clip_) return;

    int x = frameToX(playheadFrame_);
    if (x >= rect.left() && x <= rect.right()) {
        // Glow effect
        QLinearGradient glow(x - 4, 0, x + 4, 0);
        glow.setColorAt(0.0, QColor(255, 60, 60, 0));
        glow.setColorAt(0.5, QColor(255, 60, 60, 80));
        glow.setColorAt(1.0, QColor(255, 60, 60, 0));
        painter.fillRect(x - 4, rect.top(), 8, rect.height(), glow);

        // Main line
        painter.setPen(QPen(QColor(255, 80, 80), 2));
        painter.drawLine(x, rect.top(), x, rect.bottom());
    }
}

void WaveformView::drawTimeRuler(QPainter& painter, const QRect& rect) {
    // Ruler background
    QLinearGradient bg(0, rect.top(), 0, rect.bottom());
    bg.setColorAt(0.0, QColor(40, 44, 50));
    bg.setColorAt(1.0, QColor(30, 34, 40));
    painter.fillRect(rect, bg);

    // Top border
    painter.setPen(QColor(60, 65, 75));
    painter.drawLine(rect.left(), rect.top(), rect.right(), rect.top());

    if (!clip_) return;

    int sampleRate = clip_->sampleRate();
    if (sampleRate <= 0) return;

    painter.setPen(QColor(140, 145, 155));

    double secondsPerPixel = samplesPerPixel_ / sampleRate;
    double viewSeconds = width() * secondsPerPixel;

    // Choose tick interval
    double interval = 0.1;
    if (viewSeconds > 120) interval = 30.0;
    else if (viewSeconds > 60) interval = 10.0;
    else if (viewSeconds > 30) interval = 5.0;
    else if (viewSeconds > 10) interval = 1.0;
    else if (viewSeconds > 5) interval = 0.5;
    else if (viewSeconds > 2) interval = 0.2;
    else if (viewSeconds > 1) interval = 0.1;
    else interval = 0.05;

    double startTime = static_cast<double>(scrollOffsetFrames_) / sampleRate;
    double firstTick = std::ceil(startTime / interval) * interval;

    QFont smallFont = painter.font();
    smallFont.setPointSize(9);
    painter.setFont(smallFont);

    for (double t = firstTick; t < startTime + viewSeconds + interval; t += interval) {
        int frame = static_cast<int>(t * sampleRate);
        int x = frameToX(frame);

        if (x >= rect.left() && x <= rect.right()) {
            // Major tick
            painter.setPen(QColor(100, 105, 115));
            painter.drawLine(x, rect.top() + 2, x, rect.top() + 8);

            // Time label
            QString label;
            int mins = static_cast<int>(t) / 60;
            double secs = std::fmod(t, 60.0);

            if (mins > 0) {
                label = QString("%1:%2").arg(mins).arg(secs, 5, 'f', 2, '0');
            } else {
                label = QString::number(secs, 'f', 2);
            }

            painter.setPen(QColor(160, 165, 175));
            painter.drawText(x + 3, rect.bottom() - 5, label);
        }
    }
}

// ============================================================================
// Coordinate conversions
// ============================================================================

int WaveformView::frameToX(int frame) const {
    return static_cast<int>((frame - scrollOffsetFrames_) / samplesPerPixel_);
}

int WaveformView::xToFrame(int x) const {
    return scrollOffsetFrames_ + static_cast<int>(x * samplesPerPixel_);
}

// ============================================================================
// Hit testing
// ============================================================================

WaveformView::HandleHit WaveformView::hitTestHandle(int x) const {
    if (!clip_) return HandleHit::None;

    int maxFrame = static_cast<int>(clip_->frameCount());
    int effectiveEndFrame = (trimEndFrame_ > 0) ? trimEndFrame_ : maxFrame;
    int activeLength = effectiveEndFrame - trimStartFrame_;

    // Test trim handles first (higher priority)
    int xTrimStart = frameToX(trimStartFrame_);
    int xTrimEnd = frameToX(effectiveEndFrame);

    if (std::abs(x - xTrimStart) <= kHandleWidth) {
        return HandleHit::TrimStart;
    }

    if (std::abs(x - xTrimEnd) <= kHandleWidth) {
        return HandleHit::TrimEnd;
    }

    // Test fade handles (lower priority, only if fades are active)
    if (activeLength > 0) {
        int maxFadeEach = activeLength / 2;
        int clampedFadeIn = std::min(fadeInLengthFrames_, maxFadeEach);
        int clampedFadeOut = std::min(fadeOutLengthFrames_, maxFadeEach);

        if (clampedFadeIn > 0) {
            int xFadeInEnd = frameToX(trimStartFrame_ + clampedFadeIn);
            if (std::abs(x - xFadeInEnd) <= kHandleWidth) {
                return HandleHit::FadeInEnd;
            }
        }

        if (clampedFadeOut > 0) {
            int xFadeOutStart = frameToX(effectiveEndFrame - clampedFadeOut);
            if (std::abs(x - xFadeOutStart) <= kHandleWidth) {
                return HandleHit::FadeOutStart;
            }
        }
    }

    return HandleHit::None;
}

// ============================================================================
// Mouse events
// ============================================================================

void WaveformView::mousePressEvent(QMouseEvent* event) {
    if (!clip_) return;

    if (event->button() == Qt::LeftButton) {
        HandleHit hit = hitTestHandle(event->pos().x());

        if (isFadeMode_) {
            // In fade mode: dragging trim handles adjusts fade lengths
            if (hit == HandleHit::TrimStart) {
                dragMode_ = DragMode::FadeInEnd;
                dragStartX_ = event->pos().x();
                dragStartValue_ = fadeInLengthFrames_;
            } else if (hit == HandleHit::TrimEnd) {
                dragMode_ = DragMode::FadeOutStart;
                dragStartX_ = event->pos().x();
                dragStartValue_ = fadeOutLengthFrames_;
            } else {
                int frame = xToFrame(event->pos().x());
                Q_EMIT seekRequested(frame);
            }
        } else {
            // In trim mode: normal trim behavior
            if (hit == HandleHit::TrimStart) {
                dragMode_ = DragMode::TrimStart;
                dragStartX_ = event->pos().x();
                dragStartValue_ = trimStartFrame_;
            } else if (hit == HandleHit::TrimEnd) {
                dragMode_ = DragMode::TrimEnd;
                dragStartX_ = event->pos().x();
                int maxFrame = static_cast<int>(clip_->frameCount());
                dragStartValue_ = (trimEndFrame_ > 0) ? trimEndFrame_ : maxFrame;
            } else {
                int frame = xToFrame(event->pos().x());
                Q_EMIT seekRequested(frame);
            }
        }
    } else if (event->button() == Qt::MiddleButton) {
        dragMode_ = DragMode::Scroll;
        dragStartX_ = event->pos().x();
        dragStartValue_ = scrollOffsetFrames_;
        setCursor(Qt::ClosedHandCursor);
    }

    QWidget::mousePressEvent(event);
}

void WaveformView::mouseMoveEvent(QMouseEvent* event) {
    if (!clip_) return;

    int x = event->pos().x();

    if (dragMode_ == DragMode::TrimStart) {
        int newFrame = xToFrame(x);
        setTrimStartFrame(newFrame);
    } else if (dragMode_ == DragMode::TrimEnd) {
        int newFrame = xToFrame(x);
        setTrimEndFrame(newFrame);
    } else if (dragMode_ == DragMode::FadeInEnd) {
        int maxFrame = static_cast<int>(clip_->frameCount());
        int effectiveEndFrame = (trimEndFrame_ > 0) ? trimEndFrame_ : maxFrame;
        int activeLength = effectiveEndFrame - trimStartFrame_;
        int maxFadeEach = activeLength / 2;
        
        int newFrame = xToFrame(x);
        int newFadeLength = std::clamp(newFrame - trimStartFrame_, 0, maxFadeEach);
        setFadeInLengthFrames(newFadeLength);
    } else if (dragMode_ == DragMode::FadeOutStart) {
        int maxFrame = static_cast<int>(clip_->frameCount());
        int effectiveEndFrame = (trimEndFrame_ > 0) ? trimEndFrame_ : maxFrame;
        int activeLength = effectiveEndFrame - trimStartFrame_;
        int maxFadeEach = activeLength / 2;
        
        int newFrame = xToFrame(x);
        int newFadeLength = std::clamp(effectiveEndFrame - newFrame, 0, maxFadeEach);
        setFadeOutLengthFrames(newFadeLength);
    } else if (dragMode_ == DragMode::Scroll) {
        int deltaX = dragStartX_ - x;
        int deltaFrames = static_cast<int>(deltaX * samplesPerPixel_);
        int newOffset = dragStartValue_ + deltaFrames;

        int maxScroll = std::max(0, static_cast<int>(clip_->frameCount() - width() * samplesPerPixel_));
        scrollOffsetFrames_ = std::clamp(newOffset, 0, maxScroll);
        cacheValid_ = false;
        update();
    } else {
        HandleHit hit = hitTestHandle(x);
        if (hit != HandleHit::None) {
            setCursor(Qt::SizeHorCursor);
        } else {
            setCursor(Qt::ArrowCursor);
        }
    }

    QWidget::mouseMoveEvent(event);
}

void WaveformView::mouseReleaseEvent(QMouseEvent* event) {
    if (dragMode_ == DragMode::Scroll) {
        setCursor(Qt::ArrowCursor);
    }
    dragMode_ = DragMode::None;
    QWidget::mouseReleaseEvent(event);
}

void WaveformView::wheelEvent(QWheelEvent* event) {
    if (!clip_) return;

    int delta = event->angleDelta().y();

    if (event->modifiers() & Qt::ControlModifier) {
        // Zoom centered on cursor position
        int cursorX = static_cast<int>(event->position().x());
        double factor = (delta > 0) ? 0.7 : 1.4;  // Smoother zoom
        zoomAtPoint(cursorX, factor);
    } else {
        // Scroll
        int scrollAmount = static_cast<int>(delta * samplesPerPixel_ * 0.5);
        int newOffset = scrollOffsetFrames_ - scrollAmount;

        int maxScroll = std::max(0, static_cast<int>(clip_->frameCount() - width() * samplesPerPixel_));
        scrollOffsetFrames_ = std::clamp(newOffset, 0, maxScroll);
        cacheValid_ = false;
        update();
    }

    event->accept();
}
