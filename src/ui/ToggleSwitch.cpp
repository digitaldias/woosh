/**
 * @file ToggleSwitch.cpp
 * @brief Implementation of custom toggle switch widget.
 */

#include "ToggleSwitch.h"
#include <QPainter>
#include <QMouseEvent>

ToggleSwitch::ToggleSwitch(QWidget* parent)
    : QWidget(parent)
{
    setFixedSize(kTrackWidth, kTrackHeight);
    setCursor(Qt::PointingHandCursor);
    
    animation_ = new QPropertyAnimation(this, "position", this);
    animation_->setDuration(150);  // 150ms smooth animation
    animation_->setEasingCurve(QEasingCurve::InOutQuad);
}

void ToggleSwitch::setChecked(bool checked) {
    if (checked_ == checked) return;
    
    checked_ = checked;
    
    // Animate to new position
    int targetPos = checked_ ? (kTrackWidth - kThumbSize - kMargin) : 0;
    animation_->stop();
    animation_->setStartValue(position_);
    animation_->setEndValue(targetPos);
    animation_->start();
    
    Q_EMIT toggled(checked_);
}

void ToggleSwitch::setColors(const QColor& trackColorOff, const QColor& trackColorOn, const QColor& thumbColor) {
    trackColorOff_ = trackColorOff;
    trackColorOn_ = trackColorOn;
    thumbColor_ = thumbColor;
    update();
}

QSize ToggleSwitch::sizeHint() const {
    return QSize(kTrackWidth, kTrackHeight);
}

QSize ToggleSwitch::minimumSizeHint() const {
    return QSize(kTrackWidth, kTrackHeight);
}

void ToggleSwitch::setPosition(int pos) {
    position_ = pos;
    update();
}

void ToggleSwitch::paintEvent(QPaintEvent* /*event*/) {
    QPainter painter(this);
    painter.setRenderHint(QPainter::Antialiasing);
    
    // Calculate interpolation factor (0.0 = off, 1.0 = on)
    float t = static_cast<float>(position_) / (kTrackWidth - kThumbSize - kMargin);
    t = qBound(0.0f, t, 1.0f);
    
    // Interpolate track color
    QColor trackColor;
    trackColor.setRed(static_cast<int>(trackColorOff_.red() * (1.0f - t) + trackColorOn_.red() * t));
    trackColor.setGreen(static_cast<int>(trackColorOff_.green() * (1.0f - t) + trackColorOn_.green() * t));
    trackColor.setBlue(static_cast<int>(trackColorOff_.blue() * (1.0f - t) + trackColorOn_.blue() * t));
    
    // Draw track (rounded rectangle)
    int trackRadius = kTrackHeight / 2;
    painter.setPen(Qt::NoPen);
    painter.setBrush(trackColor);
    painter.drawRoundedRect(0, 0, kTrackWidth, kTrackHeight, trackRadius, trackRadius);
    
    // Draw hover overlay
    if (hovered_) {
        painter.setBrush(QColor(255, 255, 255, 20));  // Subtle white overlay
        painter.drawRoundedRect(0, 0, kTrackWidth, kTrackHeight, trackRadius, trackRadius);
    }
    
    // Draw thumb (circle)
    int thumbX = kMargin + position_;
    int thumbY = (kTrackHeight - kThumbSize) / 2;
    
    // Shadow
    painter.setBrush(QColor(0, 0, 0, 40));
    painter.drawEllipse(thumbX + 1, thumbY + 1, kThumbSize, kThumbSize);
    
    // Thumb
    painter.setBrush(thumbColor_);
    painter.drawEllipse(thumbX, thumbY, kThumbSize, kThumbSize);
}

void ToggleSwitch::mousePressEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        event->accept();
    }
}

void ToggleSwitch::mouseReleaseEvent(QMouseEvent* event) {
    if (event->button() == Qt::LeftButton) {
        setChecked(!checked_);
        event->accept();
    }
}

void ToggleSwitch::enterEvent(QEnterEvent* /*event*/) {
    hovered_ = true;
    update();
}

void ToggleSwitch::leaveEvent(QEvent* /*event*/) {
    hovered_ = false;
    update();
}
