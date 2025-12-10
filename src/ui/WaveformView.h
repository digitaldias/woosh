/**
 * @file WaveformView.h
 * @brief Waveform visualization widget with zoom, scroll, trim, and stereo display.
 *
 * Renders audio samples as a waveform with gradient fills, supports stereo
 * channel display, zooming centered on cursor, and visual trim markers.
 */

#pragma once

#include <QWidget>
#include <QString>
#include <vector>

class AudioClip;

/**
 * @class WaveformView
 * @brief Widget for visualizing and editing audio waveforms.
 *
 * Features:
 *  - Renders waveform from AudioClip samples with gradient fills
 *  - Stereo display: left channel on top, right channel on bottom
 *  - Zoom in/out centered on mouse cursor position
 *  - Horizontal scrolling when zoomed in
 *  - Trim region with draggable start/end handles
 *  - Playhead display during audio playback
 */
class WaveformView : public QWidget {
    Q_OBJECT

public:
    explicit WaveformView(QWidget* parent = nullptr);
    ~WaveformView() override;

    void setClip(AudioClip* clip);
    AudioClip* clip() const { return clip_; }

    // --- Zoom controls ---
    void zoomIn();
    void zoomOut();
    void zoomToFit();
    void zoomAtPoint(int pixelX, double factor);
    double samplesPerPixel() const { return samplesPerPixel_; }

    // --- Trim controls ---
    int trimStartFrame() const { return trimStartFrame_; }
    int trimEndFrame() const { return trimEndFrame_; }
    void setTrimStartFrame(int frame);
    void setTrimEndFrame(int frame);
    void clearTrim();
    bool hasTrimRegion() const;

    // --- Playhead ---
    void setPlayheadFrame(int frame);
    int playheadFrame() const { return playheadFrame_; }
    void clearPlayhead();

Q_SIGNALS:
    void seekRequested(int frame);
    void trimChanged(int startFrame, int endFrame);
    void trimApplyRequested();

protected:
    void paintEvent(QPaintEvent* event) override;
    void resizeEvent(QResizeEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseMoveEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void wheelEvent(QWheelEvent* event) override;

private:
    // --- Rendering helpers ---
    void computeWaveformCache();
    void drawBackground(QPainter& painter, const QRect& rect);
    void drawWaveform(QPainter& painter, const QRect& rect);
    void drawWaveformChannel(QPainter& painter, const QRect& rect, int channel, bool flipY);
    void drawTrimRegion(QPainter& painter, const QRect& rect);
    void drawPlayhead(QPainter& painter, const QRect& rect);
    void drawTimeRuler(QPainter& painter, const QRect& rect);

    // --- Coordinate conversions ---
    int frameToX(int frame) const;
    int xToFrame(int x) const;

    // --- Hit testing ---
    enum class HandleHit { None, Start, End };
    HandleHit hitTestTrimHandle(int x) const;

    // --- State ---
    AudioClip* clip_ = nullptr;

    // Zoom and scroll
    double samplesPerPixel_ = 1.0;
    int scrollOffsetFrames_ = 0;

    // Waveform cache per channel (min/max per pixel column)
    struct WaveformColumn {
        float minVal = 0.0f;
        float maxVal = 0.0f;
    };
    std::vector<std::vector<WaveformColumn>> channelCache_;  // [channel][x]
    bool cacheValid_ = false;

    // Trim region (in frames)
    int trimStartFrame_ = 0;
    int trimEndFrame_ = 0;

    // Playhead
    int playheadFrame_ = -1;

    // Drag state
    enum class DragMode { None, Scroll, TrimStart, TrimEnd };
    DragMode dragMode_ = DragMode::None;
    int dragStartX_ = 0;
    int dragStartValue_ = 0;

    // Layout constants
    static constexpr int kRulerHeight = 22;
    static constexpr int kHandleWidth = 10;
    static constexpr int kChannelGap = 2;
};
