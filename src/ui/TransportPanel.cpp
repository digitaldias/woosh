/**
 * @file TransportPanel.cpp
 * @brief Implementation of TransportPanel widget.
 */

#include "TransportPanel.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QFrame>
#include <QFont>
#include <cmath>

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

    // Use a font that supports the media control symbols
    QFont symbolFont("Segoe UI Symbol", 12);
    if (!symbolFont.exactMatch()) {
        symbolFont = QFont("Arial Unicode MS", 12);
    }

    // --- Playback controls ---
    playPauseBtn_ = new QPushButton(this);
    playPauseBtn_->setFont(symbolFont);
    playPauseBtn_->setText(QString::fromUtf8("\u25B6"));  // Play triangle ▶
    playPauseBtn_->setFixedSize(36, 32);
    playPauseBtn_->setToolTip(tr("Play / Pause (Space)"));
    connect(playPauseBtn_, &QPushButton::clicked, this, &TransportPanel::playPauseClicked);

    stopBtn_ = new QPushButton(this);
    stopBtn_->setFont(symbolFont);
    stopBtn_->setText(QString::fromUtf8("\u25A0"));  // Stop square ■
    stopBtn_->setFixedSize(36, 32);
    stopBtn_->setToolTip(tr("Stop"));
    connect(stopBtn_, &QPushButton::clicked, this, &TransportPanel::stopClicked);

    layout->addWidget(playPauseBtn_);
    layout->addWidget(stopBtn_);

    // Separator
    auto* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep1);

    // --- Zoom controls ---
    zoomOutBtn_ = new QPushButton(QString::fromUtf8("\u2212"), this);  // Minus sign −
    zoomOutBtn_->setFixedSize(32, 32);
    zoomOutBtn_->setToolTip(tr("Zoom Out"));
    connect(zoomOutBtn_, &QPushButton::clicked, this, &TransportPanel::zoomOutClicked);

    zoomInBtn_ = new QPushButton("+", this);
    zoomInBtn_->setFixedSize(32, 32);
    zoomInBtn_->setToolTip(tr("Zoom In"));
    connect(zoomInBtn_, &QPushButton::clicked, this, &TransportPanel::zoomInClicked);

    zoomFitBtn_ = new QPushButton(tr("Fit"), this);
    zoomFitBtn_->setFixedWidth(44);
    zoomFitBtn_->setToolTip(tr("Fit waveform to window"));
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
    timeLabel_ = new QLabel("0.00 / 0.00", this);
    timeLabel_->setMinimumWidth(110);
    timeLabel_->setAlignment(Qt::AlignCenter);
    QFont monoFont("Consolas", 10);
    if (!monoFont.exactMatch()) {
        monoFont = QFont("Courier New", 10);
    }
    timeLabel_->setFont(monoFont);
    layout->addWidget(timeLabel_);

    // Stretch to push trim button to the right
    layout->addStretch();

    // --- Trim controls ---
    applyTrimBtn_ = new QPushButton(tr("Apply Trim"), this);
    applyTrimBtn_->setToolTip(tr("Apply trim markers to clip"));
    applyTrimBtn_->setEnabled(false);
    connect(applyTrimBtn_, &QPushButton::clicked, this, &TransportPanel::applyTrimClicked);

    layout->addWidget(applyTrimBtn_);
}

// ============================================================================
// State updates
// ============================================================================

void TransportPanel::setPlaying(bool playing) {
    if (playing) {
        playPauseBtn_->setText(QString::fromUtf8("\u23F8"));  // Pause ⏸
        playPauseBtn_->setToolTip(tr("Pause (Space)"));
    } else {
        playPauseBtn_->setText(QString::fromUtf8("\u25B6"));  // Play ▶
        playPauseBtn_->setToolTip(tr("Play (Space)"));
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
