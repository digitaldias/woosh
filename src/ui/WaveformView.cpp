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
#include <QtMath>
#include <algorithm>
#include <cmath>

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
    // Gradient background
    QLinearGradient bg(0, rect.top(), 0, rect.bottom());
    bg.setColorAt(0.0, QColor(25, 28, 32));
    bg.setColorAt(0.5, QColor(18, 20, 24));
    bg.setColorAt(1.0, QColor(25, 28, 32));
    painter.fillRect(rect, bg);

    // Grid lines (subtle)
    painter.setPen(QColor(40, 44, 50));

    // Horizontal center line(s)
    int channels = clip_ ? clip_->channels() : 1;
    if (channels >= 2) {
        int quarterH = rect.height() / 4;
        painter.drawLine(rect.left(), rect.top() + quarterH, rect.right(), rect.top() + quarterH);
        painter.drawLine(rect.left(), rect.bottom() - quarterH, rect.right(), rect.bottom() - quarterH);
        // Channel separator
        painter.setPen(QColor(50, 55, 65));
        painter.drawLine(rect.left(), rect.center().y(), rect.right(), rect.center().y());
    } else {
        painter.drawLine(rect.left(), rect.center().y(), rect.right(), rect.center().y());
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

    // Create gradient for waveform
    QLinearGradient gradient(0, rect.top(), 0, rect.bottom());

    // Different colors for left/right channels
    if (channel == 0) {
        // Left channel - cyan/blue
        gradient.setColorAt(0.0, QColor(40, 200, 255, 200));
        gradient.setColorAt(0.5, QColor(60, 140, 200, 255));
        gradient.setColorAt(1.0, QColor(40, 200, 255, 200));
    } else {
        // Right channel - green/teal
        gradient.setColorAt(0.0, QColor(80, 220, 160, 200));
        gradient.setColorAt(0.5, QColor(50, 180, 130, 255));
        gradient.setColorAt(1.0, QColor(80, 220, 160, 200));
    }

    // Build polygon for filled waveform
    QPolygonF topLine, bottomLine;
    topLine.reserve(cache.size());
    bottomLine.reserve(cache.size());

    for (int x = 0; x < static_cast<int>(cache.size()) && x < rect.width(); ++x) {
        const auto& col = cache[x];

        float maxV = flipY ? -col.minVal : col.maxVal;
        float minV = flipY ? -col.maxVal : col.minVal;

        int yTop = centerY - static_cast<int>(maxV * halfHeight);
        int yBot = centerY - static_cast<int>(minV * halfHeight);

        topLine << QPointF(rect.left() + x, yTop);
        bottomLine.prepend(QPointF(rect.left() + x, yBot));
    }

    // Combine into closed polygon
    QPolygonF wavePolygon = topLine + bottomLine;

    painter.setPen(Qt::NoPen);
    painter.setBrush(gradient);
    painter.drawPolygon(wavePolygon);

    // Draw outline
    QPen outlinePen(channel == 0 ? QColor(100, 220, 255) : QColor(100, 230, 180), 1);
    painter.setPen(outlinePen);
    painter.setBrush(Qt::NoBrush);
    painter.drawPolyline(topLine);
    painter.drawPolyline(bottomLine);
}

void WaveformView::drawTrimRegion(QPainter& painter, const QRect& rect) {
    if (!clip_) return;

    int maxFrame = static_cast<int>(clip_->frameCount());
    int effectiveEndFrame = (trimEndFrame_ > 0) ? trimEndFrame_ : maxFrame;

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

    // Draw trim handles
    QColor handleColor(255, 180, 60);
    QColor handleLineColor(255, 200, 80);

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

WaveformView::HandleHit WaveformView::hitTestTrimHandle(int x) const {
    if (!clip_) return HandleHit::None;

    int maxFrame = static_cast<int>(clip_->frameCount());
    int effectiveEndFrame = (trimEndFrame_ > 0) ? trimEndFrame_ : maxFrame;

    int xStart = frameToX(trimStartFrame_);
    int xEnd = frameToX(effectiveEndFrame);

    if (std::abs(x - xStart) <= kHandleWidth) {
        return HandleHit::Start;
    }

    if (std::abs(x - xEnd) <= kHandleWidth) {
        return HandleHit::End;
    }

    return HandleHit::None;
}

// ============================================================================
// Mouse events
// ============================================================================

void WaveformView::mousePressEvent(QMouseEvent* event) {
    if (!clip_) return;

    if (event->button() == Qt::LeftButton) {
        HandleHit hit = hitTestTrimHandle(event->pos().x());

        if (hit == HandleHit::Start) {
            dragMode_ = DragMode::TrimStart;
            dragStartX_ = event->pos().x();
            dragStartValue_ = trimStartFrame_;
        } else if (hit == HandleHit::End) {
            dragMode_ = DragMode::TrimEnd;
            dragStartX_ = event->pos().x();
            int maxFrame = static_cast<int>(clip_->frameCount());
            dragStartValue_ = (trimEndFrame_ > 0) ? trimEndFrame_ : maxFrame;
        } else {
            int frame = xToFrame(event->pos().x());
            Q_EMIT seekRequested(frame);
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
    } else if (dragMode_ == DragMode::Scroll) {
        int deltaX = dragStartX_ - x;
        int deltaFrames = static_cast<int>(deltaX * samplesPerPixel_);
        int newOffset = dragStartValue_ + deltaFrames;

        int maxScroll = std::max(0, static_cast<int>(clip_->frameCount() - width() * samplesPerPixel_));
        scrollOffsetFrames_ = std::clamp(newOffset, 0, maxScroll);
        cacheValid_ = false;
        update();
    } else {
        HandleHit hit = hitTestTrimHandle(x);
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
