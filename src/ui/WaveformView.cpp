/**
 * @file WaveformView.cpp
 * @brief Implementation of WaveformView widget.
 */

#include "WaveformView.h"
#include "audio/AudioClip.h"

#include <QPainter>
#include <QPaintEvent>
#include <QMouseEvent>
#include <QWheelEvent>
#include <QtMath>
#include <algorithm>

// ============================================================================
// Construction
// ============================================================================

WaveformView::WaveformView(QWidget* parent)
    : QWidget(parent)
{
    setMinimumHeight(100);
    setMouseTracking(true);
    setFocusPolicy(Qt::StrongFocus);

    // Dark background
    setAutoFillBackground(true);
    QPalette pal = palette();
    pal.setColor(QPalette::Window, QColor(30, 30, 35));
    setPalette(pal);
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
    if (!clip_) return;

    // Halve samples per pixel (zoom in)
    double newSpp = samplesPerPixel_ / 2.0;
    if (newSpp < 1.0) newSpp = 1.0;  // Max zoom: 1 sample per pixel

    samplesPerPixel_ = newSpp;
    cacheValid_ = false;
    update();
}

void WaveformView::zoomOut() {
    if (!clip_) return;

    // Double samples per pixel (zoom out)
    double maxSpp = static_cast<double>(clip_->frameCount()) / width();
    double newSpp = samplesPerPixel_ * 2.0;
    if (newSpp > maxSpp) newSpp = maxSpp;

    samplesPerPixel_ = newSpp;
    cacheValid_ = false;

    // Clamp scroll offset
    int maxScroll = std::max(0, static_cast<int>(clip_->frameCount() - width() * samplesPerPixel_));
    scrollOffsetFrames_ = std::clamp(scrollOffsetFrames_, 0, maxScroll);

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

    // Background
    painter.fillRect(rect(), QColor(30, 30, 35));

    if (!clip_ || clip_->samples().empty()) {
        // Draw placeholder text
        painter.setPen(QColor(100, 100, 100));
        painter.drawText(waveformRect, Qt::AlignCenter, "Select a clip to preview");
        return;
    }

    // Compute cache if needed
    if (!cacheValid_) {
        computeWaveformCache();
    }

    // Draw in order: waveform, trim region, playhead, ruler
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
// Waveform rendering
// ============================================================================

void WaveformView::computeWaveformCache() {
    waveformCache_.clear();
    if (!clip_ || clip_->samples().empty()) {
        cacheValid_ = true;
        return;
    }

    const auto& samples = clip_->samples();
    int channels = clip_->channels();
    size_t frameCount = clip_->frameCount();
    int widgetWidth = width();

    waveformCache_.resize(widgetWidth);

    for (int x = 0; x < widgetWidth; ++x) {
        // Map pixel to frame range
        int startFrame = scrollOffsetFrames_ + static_cast<int>(x * samplesPerPixel_);
        int endFrame = scrollOffsetFrames_ + static_cast<int>((x + 1) * samplesPerPixel_);

        startFrame = std::clamp(startFrame, 0, static_cast<int>(frameCount) - 1);
        endFrame = std::clamp(endFrame, startFrame + 1, static_cast<int>(frameCount));

        float minVal = 0.0f;
        float maxVal = 0.0f;

        // Find min/max across all channels in this frame range
        for (int f = startFrame; f < endFrame; ++f) {
            for (int ch = 0; ch < channels; ++ch) {
                size_t idx = static_cast<size_t>(f * channels + ch);
                if (idx < samples.size()) {
                    float val = samples[idx];
                    minVal = std::min(minVal, val);
                    maxVal = std::max(maxVal, val);
                }
            }
        }

        waveformCache_[x] = {minVal, maxVal};
    }

    cacheValid_ = true;
}

void WaveformView::drawWaveform(QPainter& painter, const QRect& rect) {
    if (waveformCache_.empty()) return;

    int centerY = rect.center().y();
    int halfHeight = rect.height() / 2 - 4;  // Leave some margin

    // Waveform color
    QColor waveColor(80, 180, 255);
    painter.setPen(waveColor);

    for (int x = 0; x < static_cast<int>(waveformCache_.size()) && x < rect.width(); ++x) {
        const auto& col = waveformCache_[x];

        // Map sample values [-1, 1] to pixel coordinates
        int yMin = centerY - static_cast<int>(col.maxVal * halfHeight);
        int yMax = centerY - static_cast<int>(col.minVal * halfHeight);

        // Draw vertical line for this column
        painter.drawLine(rect.left() + x, yMin, rect.left() + x, yMax);
    }

    // Draw center line
    painter.setPen(QColor(60, 60, 65));
    painter.drawLine(rect.left(), centerY, rect.right(), centerY);
}

void WaveformView::drawTrimRegion(QPainter& painter, const QRect& rect) {
    if (!clip_) return;

    int maxFrame = static_cast<int>(clip_->frameCount());
    int effectiveEndFrame = (trimEndFrame_ > 0) ? trimEndFrame_ : maxFrame;

    // Draw grayed-out regions for trimmed parts
    QColor trimmedColor(0, 0, 0, 120);

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
    QColor handleColor(255, 200, 100);
    painter.setPen(QPen(handleColor, 2));

    // Start handle
    int xStart = frameToX(trimStartFrame_);
    if (xStart >= rect.left() && xStart <= rect.right()) {
        painter.drawLine(xStart, rect.top(), xStart, rect.bottom());
        // Draw triangle handle
        QPolygon startHandle;
        startHandle << QPoint(xStart, rect.top())
                    << QPoint(xStart + kHandleWidth, rect.top())
                    << QPoint(xStart, rect.top() + kHandleWidth);
        painter.setBrush(handleColor);
        painter.drawPolygon(startHandle);
    }

    // End handle
    int xEnd = frameToX(effectiveEndFrame);
    if (xEnd >= rect.left() && xEnd <= rect.right()) {
        painter.drawLine(xEnd, rect.top(), xEnd, rect.bottom());
        // Draw triangle handle
        QPolygon endHandle;
        endHandle << QPoint(xEnd, rect.top())
                  << QPoint(xEnd - kHandleWidth, rect.top())
                  << QPoint(xEnd, rect.top() + kHandleWidth);
        painter.setBrush(handleColor);
        painter.drawPolygon(endHandle);
    }
}

void WaveformView::drawPlayhead(QPainter& painter, const QRect& rect) {
    if (playheadFrame_ < 0 || !clip_) return;

    int x = frameToX(playheadFrame_);
    if (x >= rect.left() && x <= rect.right()) {
        painter.setPen(QPen(QColor(255, 80, 80), 2));
        painter.drawLine(x, rect.top(), x, rect.bottom());
    }
}

void WaveformView::drawTimeRuler(QPainter& painter, const QRect& rect) {
    if (!clip_) return;

    painter.fillRect(rect, QColor(40, 40, 45));
    painter.setPen(QColor(150, 150, 150));

    int sampleRate = clip_->sampleRate();
    if (sampleRate <= 0) return;

    // Calculate tick interval based on zoom level
    double secondsPerPixel = samplesPerPixel_ / sampleRate;
    double viewSeconds = width() * secondsPerPixel;

    // Choose appropriate interval
    double interval = 0.1;  // Default: 100ms ticks
    if (viewSeconds > 60) interval = 10.0;
    else if (viewSeconds > 30) interval = 5.0;
    else if (viewSeconds > 10) interval = 1.0;
    else if (viewSeconds > 5) interval = 0.5;
    else if (viewSeconds > 1) interval = 0.1;
    else interval = 0.05;

    // Draw ticks
    double startTime = static_cast<double>(scrollOffsetFrames_) / sampleRate;
    double firstTick = std::ceil(startTime / interval) * interval;

    QFont smallFont = painter.font();
    smallFont.setPointSize(8);
    painter.setFont(smallFont);

    for (double t = firstTick; t < startTime + viewSeconds + interval; t += interval) {
        int frame = static_cast<int>(t * sampleRate);
        int x = frameToX(frame);

        if (x >= rect.left() && x <= rect.right()) {
            painter.drawLine(x, rect.top(), x, rect.top() + 5);

            // Format time label
            QString label;
            if (interval >= 1.0) {
                int mins = static_cast<int>(t) / 60;
                int secs = static_cast<int>(t) % 60;
                label = QString("%1:%2").arg(mins).arg(secs, 2, 10, QChar('0'));
            } else {
                label = QString::number(t, 'f', 2);
            }

            painter.drawText(x + 2, rect.bottom() - 3, label);
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

    // Check start handle
    if (std::abs(x - xStart) <= kHandleWidth) {
        return HandleHit::Start;
    }

    // Check end handle
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
            // Click on waveform -> seek
            int frame = xToFrame(event->pos().x());
            Q_EMIT seekRequested(frame);
        }
    } else if (event->button() == Qt::MiddleButton) {
        // Middle button drag for scrolling
        dragMode_ = DragMode::Scroll;
        dragStartX_ = event->pos().x();
        dragStartValue_ = scrollOffsetFrames_;
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
        // Update cursor based on hover
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
    dragMode_ = DragMode::None;
    QWidget::mouseReleaseEvent(event);
}

void WaveformView::wheelEvent(QWheelEvent* event) {
    if (!clip_) return;

    // Get scroll amount
    int delta = event->angleDelta().y();

    if (event->modifiers() & Qt::ControlModifier) {
        // Ctrl+wheel = zoom
        if (delta > 0) {
            zoomIn();
        } else {
            zoomOut();
        }
    } else {
        // Regular wheel = scroll
        int scrollDelta = static_cast<int>(delta * samplesPerPixel_);
        int newOffset = scrollOffsetFrames_ - scrollDelta;

        int maxScroll = std::max(0, static_cast<int>(clip_->frameCount() - width() * samplesPerPixel_));
        scrollOffsetFrames_ = std::clamp(newOffset, 0, maxScroll);
        cacheValid_ = false;
        update();
    }

    event->accept();
}
