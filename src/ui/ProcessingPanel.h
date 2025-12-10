/**
 * @file ProcessingPanel.h
 * @brief Panel widget containing normalize and compressor controls with apply buttons.
 *
 * This panel provides input fields for normalization target and compressor parameters,
 * plus buttons to apply processing to selected clips or all clips.
 */

#pragma once

#include <QGroupBox>

class QLineEdit;
class QPushButton;

/**
 * @class ProcessingPanel
 * @brief A grouped panel for audio processing controls (normalize, compress).
 *
 * Emits signals when the user clicks "Apply to Selected" or "Apply to All".
 * The parent (MainWindow) connects these signals to perform the actual processing.
 */
class ProcessingPanel : public QGroupBox {
    Q_OBJECT

public:
    /**
     * @brief Construct the processing panel.
     * @param parent Optional parent widget.
     */
    explicit ProcessingPanel(QWidget* parent = nullptr);

    // --- Accessors for current parameter values ---

    /** @brief Get the normalize target in dBFS. */
    double normalizeTarget() const;

    /** @brief Get the compressor threshold in dB. */
    float compThreshold() const;

    /** @brief Get the compressor ratio (e.g., 4.0 means 4:1). */
    float compRatio() const;

    /** @brief Get the compressor attack time in milliseconds. */
    float compAttackMs() const;

    /** @brief Get the compressor release time in milliseconds. */
    float compReleaseMs() const;

    /** @brief Get the compressor makeup gain in dB. */
    float compMakeupDb() const;

Q_SIGNALS:
    /**
     * @brief Emitted when user clicks "Apply to Selected".
     * @param normalize If true, apply normalization.
     * @param compress If true, apply compression.
     */
    void applyToSelected(bool normalize, bool compress);

    /**
     * @brief Emitted when user clicks "Apply to All".
     * @param normalize If true, apply normalization.
     * @param compress If true, apply compression.
     */
    void applyToAll(bool normalize, bool compress);

private Q_SLOTS:
    void onApplySelectedClicked();
    void onApplyAllClicked();

private:
    void setupUi();

    // Normalize controls
    QLineEdit* normalizeTargetEdit_ = nullptr;

    // Compressor controls
    QLineEdit* thresholdEdit_ = nullptr;
    QLineEdit* ratioEdit_ = nullptr;
    QLineEdit* attackEdit_ = nullptr;
    QLineEdit* releaseEdit_ = nullptr;
    QLineEdit* makeupEdit_ = nullptr;

    // Apply buttons
    QPushButton* applySelectedBtn_ = nullptr;
    QPushButton* applyAllBtn_ = nullptr;
};

