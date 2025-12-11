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
class QCheckBox;
class ToggleSwitch;

/**
 * @class TransportPanel
 * @brief Widget containing playback and zoom controls.
 *
 * Layout:
 *  [Play/Pause] [Stop] | [Zoom-] [Zoom+] [Fit] | Time: 0:00 / 0:00 | [Apply Trim]
 */
class TransportPanel final : public QWidget {
    Q_OBJECT

public:
    explicit TransportPanel(QWidget* parent = nullptr);
    ~TransportPanel() = default;

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

    /** @brief Show full clip extent checkbox toggled. */
    void showFullExtentChanged(bool show);

    /** @brief Edit mode toggled between trim and fade. */
    void editModeChanged(bool isFadeMode);

private:
    void setupUi();
    QString formatTime(double seconds) const;

    QPushButton* playPauseBtn_ = nullptr;
    QPushButton* stopBtn_ = nullptr;
    QPushButton* zoomInBtn_ = nullptr;
    QPushButton* zoomOutBtn_ = nullptr;
    QPushButton* zoomFitBtn_ = nullptr;
    QPushButton* applyTrimBtn_ = nullptr;
    ToggleSwitch* modeToggleSwitch_ = nullptr;
    QLabel* timeLabel_ = nullptr;
    QCheckBox* showFullExtentCheck_ = nullptr;
};

