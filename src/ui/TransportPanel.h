/**
 * @file TransportPanel.h
 * @brief Transport controls (play/pause/stop) and zoom controls for waveform.
 *
 * Provides playback controls, zoom in/out/fit buttons, and trim apply button.
 */

#pragma once

#include <QWidget>

class QPushButton;
class QLabel;

/**
 * @class TransportPanel
 * @brief Widget containing playback and zoom controls.
 *
 * Layout:
 *  [Play/Pause] [Stop] | [Zoom-] [Zoom+] [Fit] | Time: 0:00 / 0:00 | [Apply Trim]
 */
class TransportPanel : public QWidget {
    Q_OBJECT

public:
    explicit TransportPanel(QWidget* parent = nullptr);
    ~TransportPanel() override = default;

    /**
     * @brief Update the play/pause button state.
     * @param playing True if currently playing.
     */
    void setPlaying(bool playing);

    /**
     * @brief Update the time display.
     * @param currentSecs Current position in seconds.
     * @param totalSecs Total duration in seconds.
     */
    void setTimeDisplay(double currentSecs, double totalSecs);

    /**
     * @brief Enable or disable trim button.
     * @param enabled Whether trim is available.
     */
    void setTrimEnabled(bool enabled);

Q_SIGNALS:
    /** @brief Play/Pause button clicked. */
    void playPauseClicked();

    /** @brief Stop button clicked. */
    void stopClicked();

    /** @brief Zoom in button clicked. */
    void zoomInClicked();

    /** @brief Zoom out button clicked. */
    void zoomOutClicked();

    /** @brief Fit to window button clicked. */
    void zoomFitClicked();

    /** @brief Apply trim button clicked. */
    void applyTrimClicked();

private:
    void setupUi();
    QString formatTime(double seconds) const;

    QPushButton* playPauseBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* zoomInBtn_ = nullptr;
    QPushButton* zoomOutBtn_ = nullptr;
    QPushButton* zoomFitBtn_ = nullptr;
    QPushButton* applyTrimBtn_ = nullptr;
    QLabel* timeLabel_ = nullptr;
};

