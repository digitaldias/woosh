#pragma once

#include <QElapsedTimer>
#include <QFrame>

class QTimer;

class VuMeterWidget : public QFrame {
    Q_OBJECT

public:
    explicit VuMeterWidget(QWidget* parent = nullptr);

public Q_SLOTS:
    void setLevels(float left, float right);

protected:
    void paintEvent(QPaintEvent* event) override;

private Q_SLOTS:
    void onUpdate();

private:
    float targetLeft_ = 0.0f;
    float targetRight_ = 0.0f;

    float currentLeft_ = 0.0f;
    float currentRight_ = 0.0f;

    float peakLeft_ = 0.0f;
    float peakRight_ = 0.0f;

    QTimer* timer_ = nullptr;
    QElapsedTimer elapsed_;

    qint64 lastPeakLeftMs_ = 0;
    qint64 lastPeakRightMs_ = 0;

    static constexpr int kUpdateIntervalMs = 30;
    static constexpr int kPeakHoldMs = 600;
    static const float kAttackCoeff;
    static const float kReleaseCoeff;
    static const float kPeakFalloffCoeff;

    QColor levelColor(float value) const;
};
