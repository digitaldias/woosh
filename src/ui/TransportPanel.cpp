/**
 * @file TransportPanel.cpp
 * @brief Implementation of TransportPanel widget.
 */

#include "TransportPanel.h"
#include "ToggleSwitch.h"

#include <QHBoxLayout>
#include <QPushButton>
#include <QLabel>
#include <QCheckBox>
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
    layout->setContentsMargins(8, 8, 8, 8);
    layout->setSpacing(6);

    // Use a font that supports the media control symbols
    QFont symbolFont("Segoe UI Symbol", 16);
    if (!symbolFont.exactMatch()) {
        symbolFont = QFont("Arial Unicode MS", 16);
    }

    // --- Playback controls ---
    playPauseBtn_ = new QPushButton(this);
    playPauseBtn_->setFont(symbolFont);
    playPauseBtn_->setText(QString::fromUtf8("\u25B6"));  // Play triangle ▶
    playPauseBtn_->setFixedSize(48, 40);
    playPauseBtn_->setToolTip(tr("Play / Pause (Space)"));
    playPauseBtn_->setProperty("class", "transport");
    connect(playPauseBtn_, &QPushButton::clicked, this, &TransportPanel::playPauseClicked);

    stopBtn_ = new QPushButton(this);
    stopBtn_->setFont(symbolFont);
    stopBtn_->setText(QString::fromUtf8("\u25A0"));  // Stop square ■
    stopBtn_->setFixedSize(48, 40);
    stopBtn_->setToolTip(tr("Stop"));
    stopBtn_->setProperty("class", "transport");
    connect(stopBtn_, &QPushButton::clicked, this, &TransportPanel::stopClicked);

    layout->addWidget(playPauseBtn_);
    layout->addWidget(stopBtn_);

    // Separator
    auto* sep1 = new QFrame(this);
    sep1->setFrameShape(QFrame::VLine);
    sep1->setFrameShadow(QFrame::Sunken);
    layout->addWidget(sep1);

    // --- Zoom controls ---
    QFont zoomFont("Segoe UI", 14, QFont::Bold);
    
    zoomOutBtn_ = new QPushButton(QString::fromUtf8("\u2212"), this);  // Minus sign −
    zoomOutBtn_->setFont(zoomFont);
    zoomOutBtn_->setFixedSize(44, 40);
    zoomOutBtn_->setToolTip(tr("Zoom Out"));
    zoomOutBtn_->setProperty("class", "transport");
    connect(zoomOutBtn_, &QPushButton::clicked, this, &TransportPanel::zoomOutClicked);

    zoomInBtn_ = new QPushButton("+", this);
    zoomInBtn_->setFont(zoomFont);
    zoomInBtn_->setFixedSize(44, 40);
    zoomInBtn_->setToolTip(tr("Zoom In"));
    zoomInBtn_->setProperty("class", "transport");
    connect(zoomInBtn_, &QPushButton::clicked, this, &TransportPanel::zoomInClicked);

    zoomFitBtn_ = new QPushButton(tr("Fit"), this);
    zoomFitBtn_->setFont(QFont("Segoe UI", 11, QFont::Bold));
    zoomFitBtn_->setFixedSize(52, 40);
    zoomFitBtn_->setToolTip(tr("Fit waveform to window"));
    zoomFitBtn_->setProperty("class", "transport");
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
    timeLabel_->setMinimumWidth(120);
    timeLabel_->setAlignment(Qt::AlignCenter);
    QFont monoFont("Consolas", 12);
    if (!monoFont.exactMatch()) {
        monoFont = QFont("Courier New", 12);
    }
    timeLabel_->setFont(monoFont);
    layout->addWidget(timeLabel_);

    // Stretch to push trim button to the right
    layout->addStretch();

    // --- Trim controls ---
    showFullExtentCheck_ = new QCheckBox(tr("Show full clip extent"), this);
    showFullExtentCheck_->setChecked(true);
    showFullExtentCheck_->setToolTip(tr("Show entire clip including trimmed regions"));
    connect(showFullExtentCheck_, &QCheckBox::toggled, this, &TransportPanel::showFullExtentChanged);

    // Mode toggle with label
    auto* modeLabel = new QLabel(tr("Trim"), this);
    modeLabel->setAlignment(Qt::AlignCenter);
    modeLabel->setMinimumWidth(40);
    
    modeToggleSwitch_ = new ToggleSwitch(this);
    modeToggleSwitch_->setToolTip(tr("Toggle between Trim and Fade modes"));
    modeToggleSwitch_->setColors(
        QColor(100, 100, 100),   // Gray when off (Trim)
        QColor(0, 200, 255),     // Cyan when on (Fade)
        QColor(255, 255, 255)    // White thumb
    );
    connect(modeToggleSwitch_, &ToggleSwitch::toggled, this, [this, modeLabel](bool checked) {
        modeLabel->setText(checked ? tr("Fade") : tr("Trim"));
        // Hide Apply button in fade mode (fades are non-destructive)
        applyTrimBtn_->setVisible(!checked);
        Q_EMIT editModeChanged(checked);
    });

    applyTrimBtn_ = new QPushButton(tr("Apply"), this);
    applyTrimBtn_->setToolTip(tr("Apply trim to clip"));
    applyTrimBtn_->setEnabled(false);
    applyTrimBtn_->setProperty("class", "primary");
    applyTrimBtn_->setMinimumHeight(36);
    connect(applyTrimBtn_, &QPushButton::clicked, this, &TransportPanel::applyTrimClicked);

    layout->addWidget(showFullExtentCheck_);
    layout->addWidget(modeLabel);
    layout->addWidget(modeToggleSwitch_);
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
