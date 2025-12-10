/**
 * @file TransportPanel.cpp
 * @brief Implementation of TransportPanel widget.
 */

#include "TransportPanel.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>

// ============================================================================
// Construction
// ============================================================================

TransportPanel::TransportPanel(QWidget* parent)
    : QWidget(parent)
{
    setupUi();
}

// ============================================================================
// UI Setup
// ============================================================================

void TransportPanel::setupUi() {
    auto* layout = new QHBoxLayout(this);
    layout->setContentsMargins(4, 4, 4, 4);
    layout->setSpacing(4);

    // --- Playback controls ---
    playPauseBtn_ = new QPushButton(QStringLiteral("\u25B6"), this);  // Play triangle
    playPauseBtn_->setFixedSize(32, 28);
    playPauseBtn_->setToolTip("Play / Pause (Space)");
    connect(playPauseBtn_, &QPushButton::clicked, this, &TransportPanel::playPauseClicked);

    stopBtn_ = new QPushButton(QStringLiteral("\u25A0"), this);  // Stop square
    stopBtn_->setFixedSize(32, 28);
    stopBtn_->setToolTip("Stop");
    connect(stopBtn_, &QPushButton::clicked, this, &TransportPanel::stopClicked);

    layout->addWidget(playPauseBtn_);
    layout->addWidget(stopBtn_);

    // Separator
    auto* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep1);

    // --- Zoom controls ---
    zoomOutBtn_ = new QPushButton("-", this);
    zoomOutBtn_->setFixedSize(28, 28);
    zoomOutBtn_->setToolTip("Zoom Out (Ctrl+-)");
    connect(zoomOutBtn_, &QPushButton::clicked, this, &TransportPanel::zoomOutClicked);

    zoomInBtn_ = new QPushButton("+", this);
    zoomInBtn_->setFixedSize(28, 28);
    zoomInBtn_->setToolTip("Zoom In (Ctrl++)");
    connect(zoomInBtn_, &QPushButton::clicked, this, &TransportPanel::zoomInClicked);

    zoomFitBtn_ = new QPushButton("Fit", this);
    zoomFitBtn_->setFixedWidth(40);
    zoomFitBtn_->setToolTip("Fit waveform to window");
    connect(zoomFitBtn_, &QPushButton::clicked, this, &TransportPanel::zoomFitClicked);

    layout->addWidget(zoomOutBtn_);
    layout->addWidget(zoomInBtn_);
    layout->addWidget(zoomFitBtn_);

    // Separator
    auto* sep2 = new QFrame(this);
    sep2->setFrameShape(QFrame::VLine);
    sep2->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep2);

    // --- Time display ---
    timeLabel_ = new QLabel("0:00 / 0:00", this);
    timeLabel_->setMinimumWidth(100);
    timeLabel_->setAlignment(Qt::AlignCenter);
    layout->addWidget(timeLabel_);

    // Stretch to push trim button to the right
    layout->addStretch();

    // --- Trim controls ---
    applyTrimBtn_ = new QPushButton("Apply Trim", this);
    applyTrimBtn_->setToolTip("Apply trim markers to clip");
    applyTrimBtn_->setEnabled(false);
    connect(applyTrimBtn_, &QPushButton::clicked, this, &TransportPanel::applyTrimClicked);

    layout->addWidget(applyTrimBtn_);
}

// ============================================================================
// State updates
// ============================================================================

void TransportPanel::setPlaying(bool playing) {
    if (playing) {
        playPauseBtn_->setText(QStringLiteral("\u2016"));  // Pause bars
        playPauseBtn_->setToolTip("Pause (Space)");
    } else {
        playPauseBtn_->setText(QStringLiteral("\u25B6"));  // Play triangle
        playPauseBtn_->setToolTip("Play (Space)");
    }
}

void TransportPanel::setTimeDisplay(double currentSecs, double totalSecs) {
    QString current = formatTime(currentSecs);
    QString total = formatTime(totalSecs);
    timeLabel_->setText(QString("%1 / %2").arg(current, total));
}

void TransportPanel::setTrimEnabled(bool enabled) {
    applyTrimBtn_->setEnabled(enabled);
}

// ============================================================================
// Helpers
// ============================================================================

QString TransportPanel::formatTime(double seconds) const {
    if (seconds < 0) seconds = 0;

    int mins = static_cast<int>(seconds) / 60;
    int secs = static_cast<int>(seconds) % 60;
    int ms = static_cast<int>((seconds - std::floor(seconds)) * 100);

    if (mins > 0) {
        return QString("%1:%2.%3")
            .arg(mins)
            .arg(secs, 2, 10, QChar('0'))
            .arg(ms, 2, 10, QChar('0'));
    } else {
        return QString("%1.%2")
            .arg(secs)
            .arg(ms, 2, 10, QChar('0'));
    }
}

