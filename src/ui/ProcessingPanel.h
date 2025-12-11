/**
 * @file ProcessingPanel.h
 * @brief Panel widget containing normalize and compressor controls with apply buttons.
 *
 * This panel provides input fields for normalization target and compressor parameters,
 * plus separate buttons to apply normalization or compression independently.
 */

#pragma once

#include <QGroupBox>

class QLineEdit;
class QPushButton;

/**
 * @class ProcessingPanel
 * @brief A grouped panel for audio processing controls (normalize, compress).
 *
 * Emits signals when the user clicks normalize or compress buttons.
 * Operations can be applied to selected clips or all clips independently.
 */
class ProcessingPanel final : public QGroupBox {
    Q_OBJECT

public:
    /**
     * @brief Construct the processing panel.
     * @param parent Optional parent widget.
     */
    explicit ProcessingPanel(QWidget* parent = nullptr);

    // --- Accessors for current parameter values ---

    /** @brief Get the normalize target in dBFS. */
    [[nodiscard]] double normalizeTarget() const;

    /** @brief Get the compressor threshold in dB. */
    [[nodiscard]] float compThreshold() const;

    /** @brief Get the compressor ratio (e.g., 4.0 means 4:1). */
    [[nodiscard]] float compRatio() const;

    /** @brief Get the compressor attack time in milliseconds. */
    [[nodiscard]] float compAttackMs() const;

    /** @brief Get the compressor release time in milliseconds. */
    [[nodiscard]] float compReleaseMs() const;

    /** @brief Get the compressor makeup gain in dB. */
    [[nodiscard]] float compMakeupDb() const;

Q_SIGNALS:
    /**
     * @brief Emitted when user clicks "Normalize Selected".
     */
    void normalizeSelectedRequested();

    /**
     * @brief Emitted when user clicks "Normalize All".
     */
    void normalizeAllRequested();

    /**
     * @brief Emitted when user clicks "Compress Selected".
     */
    void compressSelectedRequested();

    /**
     * @brief Emitted when user clicks "Compress All".
     */
    void compressAllRequested();

private:
    void setupUi();

    // Normalize controls
    QLineEdit* normalizeTargetEdit_ = nullptr;
    QPushButton* normalizeSelectedBtn_ = nullptr;
    QPushButton* normalizeAllBtn_ = nullptr;

    // Compressor controls
    QLineEdit* thresholdEdit_ = nullptr;
    QLineEdit* ratioEdit_ = nullptr;
    QLineEdit* attackEdit_ = nullptr;
    QLineEdit* releaseEdit_ = nullptr;
    QLineEdit* makeupEdit_ = nullptr;
    QPushButton* compressSelectedBtn_ = nullptr;
    QPushButton* compressAllBtn_ = nullptr;
};

