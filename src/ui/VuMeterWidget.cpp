#include "VuMeterWidget.h"

#include <QPainter>
#include <QTimer>
#include <QtMath>

constexpr float VuMeterWidget::kAttackCoeff = 0.4f;
constexpr float VuMeterWidget::kReleaseCoeff = 0.15f;
constexpr float VuMeterWidget::kPeakFalloffCoeff = 0.2f;

VuMeterWidget::VuMeterWidget(QWidget* parent)
    : QFrame(parent)
{
    setMinimumSize(70, 200);
    setFixedWidth(70);

    timer_ = new QTimer(this);
    timer_->setInterval(kUpdateIntervalMs);
    connect(timer_, &QTimer::timeout, this, &VuMeterWidget::onUpdate);
    timer_->start();
    elapsed_.start();
}

void VuMeterWidget::setLevels(float left, float right) {
    left = std::clamp(left, 0.0f, 1.0f);
    right = std::clamp(right, 0.0f, 1.0f);

    targetLeft_ = left;
    targetRight_ = right;

    const qint64 now = elapsed_.elapsed();
    if (left > peakLeft_) {
        peakLeft_ = left;
        lastPeakLeftMs_ = now;
    }
    if (right > peakRight_) {
        peakRight_ = right;
        lastPeakRightMs_ = now;
    }
}

void VuMeterWidget::onUpdate() {
    const qint64 now = elapsed_.elapsed();

    auto smooth = [](float current, float target, float attack, float release) {
        if (target > current) {
            float coeff = attack;
            return current + (target - current) * coeff;
        } else {
            float coeff = release;
            return current + (target - current) * coeff;
        }
    };

    currentLeft_ = smooth(currentLeft_, targetLeft_, kAttackCoeff, kReleaseCoeff);
    currentRight_ = smooth(currentRight_, targetRight_, kAttackCoeff, kReleaseCoeff);

    if (now - lastPeakLeftMs_ > kPeakHoldMs) {
        peakLeft_ = smooth(peakLeft_, currentLeft_, kPeakFalloffCoeff, kPeakFalloffCoeff);
    }

    if (now - lastPeakRightMs_ > kPeakHoldMs) {
        peakRight_ = smooth(peakRight_, currentRight_, kPeakFalloffCoeff, kPeakFalloffCoeff);
    }

    update();
}

QColor VuMeterWidget::levelColor(float value) const {
    if (value < 0.25f) {
        return QColor(0, 200, 0);
    }
    if (value < 0.7f) {
        return QColor(200, 200, 0);
    }
    return QColor(200, 0, 0);
}

void VuMeterWidget::paintEvent(QPaintEvent* event) {
    Q_UNUSED(event);

    QPainter p(this);
    p.setRenderHint(QPainter::Antialiasing, true);

    const QRectF r = rect();
    p.fillRect(r, QColor(30, 30, 30));

    // Layout: [scale labels] [left bar] [right bar]
    const int topMargin = 18;    // space for peak dB readout
    const int bottomMargin = 4;
    const int leftMargin = 28;   // space for dB scale labels
    const int rightMargin = 4;
    const int barSpacing = 2;

    const int barAreaWidth = r.width() - leftMargin - rightMargin;
    const int barWidth = (barAreaWidth - barSpacing) / 2;
    const int barHeight = r.height() - topMargin - bottomMargin;

    if (barWidth <= 0 || barHeight <= 0) return;

    // dB scale: 0 dB at top, -60 dB at bottom
    const float minDb = -60.0f;
    const float maxDb = 0.0f;

    // Convert linear level to dB position (0..1 where 1 = top)
    auto linearToDbPos = [minDb, maxDb](float linear) -> float {
        if (linear <= 0.0001f) return 0.0f; // -80 dB or below
        float db = 20.0f * std::log10(linear);
        db = std::clamp(db, minDb, maxDb);
        return (db - minDb) / (maxDb - minDb);
    };

    // Draw dB scale labels and tick marks on the left
    p.setFont(QFont("Arial", 7));
    p.setPen(QColor(180, 180, 180));

    const std::vector<int> dbMarks = {0, -6, -12, -18, -24, -30, -36, -42, -48, -54, -60};
    for (int db : dbMarks) {
        float pos = static_cast<float>(db - minDb) / (maxDb - minDb);
        int y = topMargin + static_cast<int>((1.0f - pos) * barHeight);

        // Tick mark
        p.drawLine(leftMargin - 4, y, leftMargin - 1, y);

        // Label
        QString label = QString::number(db);
        QRect textRect(0, y - 6, leftMargin - 6, 12);
        p.drawText(textRect, Qt::AlignRight | Qt::AlignVCenter, label);
    }

    // Draw bars with gradient coloring
    auto drawBar = [&](float level, float peak, int xOffset) {
        QRectF barRect(leftMargin + xOffset, topMargin, barWidth, barHeight);

        // Background
        p.fillRect(barRect, QColor(20, 20, 20));

        // Level fill from bottom up using dB scale
        const float dbPos = linearToDbPos(level);
        const float h = barHeight * dbPos;

        if (h > 0) {
            QRectF fillRect = barRect;
            fillRect.setTop(barRect.bottom() - h);

            // Gradient: green at bottom, yellow in middle, red at top
            QLinearGradient grad(fillRect.topLeft(), fillRect.bottomLeft());
            grad.setColorAt(0.0, QColor(255, 50, 50));   // red at top (0 dB)
            grad.setColorAt(0.3, QColor(255, 200, 0));   // yellow around -18 dB
            grad.setColorAt(1.0, QColor(0, 200, 0));     // green at bottom

            p.fillRect(fillRect, grad);
        }

        // Peak indicator line
        const float peakDbPos = linearToDbPos(peak);
        if (peakDbPos > 0.01f) {
            const qreal peakY = barRect.bottom() - barHeight * peakDbPos;
            p.setPen(QPen(Qt::white, 2));
            p.drawLine(QPointF(barRect.left(), peakY), QPointF(barRect.right(), peakY));
        }
    };

    drawBar(currentLeft_, peakLeft_, 0);
    drawBar(currentRight_, peakRight_, barWidth + barSpacing);

    // Draw peak dB readout at top
    float peakDb = 20.0f * std::log10(std::max(peakLeft_, peakRight_) + 0.0001f);
    peakDb = std::clamp(peakDb, minDb, maxDb);

    p.setFont(QFont("Arial", 9, QFont::Bold));
    p.setPen(peakDb > -6.0f ? QColor(255, 80, 80) : QColor(200, 200, 200));
    QString peakText = QString::number(peakDb, 'f', 1);
    p.drawText(QRect(0, 2, r.width(), 14), Qt::AlignHCenter | Qt::AlignTop, peakText);

    // Draw 3D sunken border effect with high contrast
    const int w = static_cast<int>(r.width());
    const int ht = static_cast<int>(r.height());

    // Outer shadow (top and left) - very dark
    p.setPen(QPen(QColor(0, 0, 0), 1));
    p.drawLine(0, 0, w - 1, 0);           // top
    p.drawLine(0, 0, 0, ht - 1);          // left

    // Inner shadow
    p.setPen(QPen(QColor(10, 10, 10), 1));
    p.drawLine(1, 1, w - 2, 1);           // top inner
    p.drawLine(1, 1, 1, ht - 2);          // left inner

    // Third shadow line for deeper effect
    p.setPen(QPen(QColor(20, 20, 20), 1));
    p.drawLine(2, 2, w - 3, 2);           // top inner 2
    p.drawLine(2, 2, 2, ht - 3);          // left inner 2

    // Outer highlight (bottom and right) - bright
    p.setPen(QPen(QColor(100, 100, 100), 1));
    p.drawLine(0, ht - 1, w - 1, ht - 1); // bottom
    p.drawLine(w - 1, 0, w - 1, ht - 1);  // right

    // Inner highlight
    p.setPen(QPen(QColor(80, 80, 80), 1));
    p.drawLine(1, ht - 2, w - 2, ht - 2); // bottom inner
    p.drawLine(w - 2, 1, w - 2, ht - 2);  // right inner

    // Third highlight line
    p.setPen(QPen(QColor(60, 60, 60), 1));
    p.drawLine(2, ht - 3, w - 3, ht - 3); // bottom inner 2
    p.drawLine(w - 3, 2, w - 3, ht - 3);  // right inner 2
}
