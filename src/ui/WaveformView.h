/**
 * @file WaveformView.h
 * @brief Waveform visualization widget with zoom, scroll, and trim support.
 *
 * Renders audio samples as a waveform, supports zooming in/out, scrolling,
 * and visual trim region selection. Also displays playhead during playback.
 */

#pragma once

#include <QWidget>
#include <QString>
#include <vector>

class AudioClip;
class QScrollBar;

/**
 * @class WaveformView
 * @brief Widget for visualizing and editing audio waveforms.
 *
 * Features:
 *  - Renders waveform from AudioClip samples
 *  - Zoom in/out with mouse wheel or controls
 *  - Horizontal scrolling when zoomed in
 *  - Trim region with draggable start/end handles
 *  - Playhead display during audio playback
 */
class WaveformView : public QWidget {
    Q_OBJECT

public:
    explicit WaveformView(QWidget* parent = nullptr);
    ~WaveformView() override;

    /**
     * @brief Set the audio clip to display.
     * @param clip Pointer to the clip (nullptr clears the view).
     */
    void setClip(AudioClip* clip);

    /**
     * @brief Get the current clip being displayed.
     */
    AudioClip* clip() const { return clip_; }

    // --- Zoom controls ---

    /** @brief Zoom in (show fewer samples). */
    void zoomIn();

    /** @brief Zoom out (show more samples). */
    void zoomOut();

    /** @brief Fit entire clip in view. */
    void zoomToFit();

    /** @brief Get current zoom level (samples per pixel). */
    double samplesPerPixel() const { return samplesPerPixel_; }

    // --- Trim controls ---

    /** @brief Get trim start position in frames. */
    int trimStartFrame() const { return trimStartFrame_; }

    /** @brief Get trim end position in frames (0 = no end trim). */
    int trimEndFrame() const { return trimEndFrame_; }

    /** @brief Set trim start position in frames. */
    void setTrimStartFrame(int frame);

    /** @brief Set trim end position in frames. */
    void setTrimEndFrame(int frame);

    /** @brief Reset trim to show entire clip. */
    void clearTrim();

    /** @brief Check if trim region is set. */
    bool hasTrimRegion() const;

    // --- Playhead ---

    /**
     * @brief Set playhead position for display.
     * @param frame Current playback position in frames.
     */
    void setPlayheadFrame(int frame);

    /** @brief Get current playhead position. */
    int playheadFrame() const { return playheadFrame_; }

    /** @brief Clear playhead (hide it). */
    void clearPlayhead();

Q_SIGNALS:
    /** @brief Emitted when user clicks on the waveform (for seek). */
    void seekRequested(int frame);

    /** @brief Emitted when trim region changes. */
    void trimChanged(int startFrame, int endFrame);

    /** @brief Emitted when user requests trim to be applied. */
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

    /** @brief Compute min/max samples for display (downsampling). */
    void computeWaveformCache();

    /** @brief Draw the waveform. */
    void drawWaveform(QPainter& painter, const QRect& rect);

    /** @brief Draw trim region overlay. */
    void drawTrimRegion(QPainter& painter, const QRect& rect);

    /** @brief Draw playhead line. */
    void drawPlayhead(QPainter& painter, const QRect& rect);

    /** @brief Draw time ruler at bottom. */
    void drawTimeRuler(QPainter& painter, const QRect& rect);

    // --- Coordinate conversions ---

    /** @brief Convert frame number to X pixel coordinate. */
    int frameToX(int frame) const;

    /** @brief Convert X pixel coordinate to frame number. */
    int xToFrame(int x) const;

    // --- Trim handle hit testing ---

    enum class HandleHit { None, Start, End };
    HandleHit hitTestTrimHandle(int x) const;

    // --- State ---
    AudioClip* clip_ = nullptr;

    // Zoom and scroll
    double samplesPerPixel_ = 1.0;       ///< Zoom level (samples per pixel)
    int scrollOffsetFrames_ = 0;         ///< Horizontal scroll position in frames

    // Waveform cache (min/max per pixel column)
    struct WaveformColumn {
        float minVal = 0.0f;
        float maxVal = 0.0f;
    };
    std::vector<WaveformColumn> waveformCache_;
    bool cacheValid_ = false;

    // Trim region (in frames)
    int trimStartFrame_ = 0;
    int trimEndFrame_ = 0;              ///< 0 means no end trim (use full clip)

    // Playhead
    int playheadFrame_ = -1;            ///< -1 = hidden

    // Drag state
    enum class DragMode { None, Scroll, TrimStart, TrimEnd };
    DragMode dragMode_ = DragMode::None;
    int dragStartX_ = 0;
    int dragStartValue_ = 0;

    // Layout constants
    static constexpr int kRulerHeight = 20;
    static constexpr int kHandleWidth = 8;
};
