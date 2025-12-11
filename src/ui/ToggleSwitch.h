/**
 * @file ToggleSwitch.h
 * @brief Custom toggle switch widget (iOS/Material Design style).
 */

#pragma once

#include <QWidget>
#include <QPropertyAnimation>

/**
 * @class ToggleSwitch
 * @brief A custom toggle switch widget with smooth sliding animation.
 * 
 * Provides an iOS/Material Design style toggle with a sliding thumb
 * that animates between on/off positions.
 */
class ToggleSwitch : public QWidget {
    Q_OBJECT
    Q_PROPERTY(int position READ position WRITE setPosition)

public:
    explicit ToggleSwitch(QWidget* parent = nullptr);
    ~ToggleSwitch() override = default;

    /**
     * @brief Check if the toggle is in the "on" state.
     */
    bool isChecked() const { return checked_; }

    /**
     * @brief Set the toggle state.
     * @param checked True for on, false for off.
     */
    void setChecked(bool checked);

    /**
     * @brief Set colors for the toggle.
     * @param trackColorOff Track color when off.
     * @param trackColorOn Track color when on.
     * @param thumbColor Thumb (circle) color.
     */
    void setColors(const QColor& trackColorOff, const QColor& trackColorOn, const QColor& thumbColor);

    QSize sizeHint() const override;
    QSize minimumSizeHint() const override;

Q_SIGNALS:
    /**
     * @brief Emitted when the toggle state changes.
     * @param checked New state (true = on, false = off).
     */
    void toggled(bool checked);

protected:
    void paintEvent(QPaintEvent* event) override;
    void mousePressEvent(QMouseEvent* event) override;
    void mouseReleaseEvent(QMouseEvent* event) override;
    void enterEvent(QEnterEvent* event) override;
    void leaveEvent(QEvent* event) override;

private:
    int position() const { return position_; }
    void setPosition(int pos);

    bool checked_ = false;
    bool hovered_ = false;
    int position_ = 0;  // Animated position (0 = off, max = on)
    
    QPropertyAnimation* animation_;
    
    // Colors
    QColor trackColorOff_{100, 100, 100};  // Gray
    QColor trackColorOn_{224, 160, 48};    // Yellow/orange
    QColor thumbColor_{255, 255, 255};     // White
    
    // Dimensions
    static constexpr int kTrackWidth = 50;
    static constexpr int kTrackHeight = 24;
    static constexpr int kThumbSize = 20;
    static constexpr int kMargin = 2;
};
