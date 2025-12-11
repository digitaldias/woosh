/**
 * @file ProcessingPanel.cpp
 * @brief Implementation of the ProcessingPanel widget.
 */

#include "ProcessingPanel.h"

#include <QFormLayout>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

ProcessingPanel::ProcessingPanel(QWidget* parent)
    : QGroupBox(tr("Processing"), parent)
{
    setupUi();
}

void ProcessingPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // --- Normalize section ---
    auto* normLabel = new QLabel(tr("<b>Normalize</b>"), this);
    mainLayout->addWidget(normLabel);

    auto* normLayout = new QHBoxLayout();
    normLayout->setContentsMargins(10, 0, 0, 0);
    normLayout->addWidget(new QLabel(tr("Target:"), this));
    normalizeTargetEdit_ = new QLineEdit("-1.0", this);
    normalizeTargetEdit_->setToolTip(tr("Target peak level in dBFS (e.g., -1.0)"));
    normalizeTargetEdit_->setFixedWidth(60);
    normLayout->addWidget(normalizeTargetEdit_);
    normLayout->addWidget(new QLabel(tr("dBFS"), this));
    normLayout->addStretch();
    mainLayout->addLayout(normLayout);

    // Normalize buttons
    auto* normBtnLayout = new QHBoxLayout();
    normBtnLayout->setContentsMargins(10, 0, 0, 0);
    normalizeSelectedBtn_ = new QPushButton(tr("Normalize Selected"), this);
    normalizeSelectedBtn_->setToolTip(tr("Normalize selected clips to target level"));
    normBtnLayout->addWidget(normalizeSelectedBtn_);
    normalizeAllBtn_ = new QPushButton(tr("Normalize All"), this);
    normalizeAllBtn_->setToolTip(tr("Normalize all clips to target level"));
    normBtnLayout->addWidget(normalizeAllBtn_);
    normBtnLayout->addStretch();
    mainLayout->addLayout(normBtnLayout);

    mainLayout->addSpacing(10);

    // --- Compressor section ---
    auto* compLabel = new QLabel(tr("<b>Compressor</b>"), this);
    mainLayout->addWidget(compLabel);

    auto* compForm = new QFormLayout();
    compForm->setContentsMargins(10, 0, 0, 0);

    thresholdEdit_ = new QLineEdit("-12.0", this);
    thresholdEdit_->setToolTip(tr("Threshold in dB where compression begins"));
    thresholdEdit_->setFixedWidth(60);
    compForm->addRow(tr("Threshold (dB):"), thresholdEdit_);

    ratioEdit_ = new QLineEdit("4.0", this);
    ratioEdit_->setToolTip(tr("Compression ratio (e.g., 4.0 means 4:1)"));
    ratioEdit_->setFixedWidth(60);
    compForm->addRow(tr("Ratio:"), ratioEdit_);

    attackEdit_ = new QLineEdit("10.0", this);
    attackEdit_->setToolTip(tr("Attack time in milliseconds"));
    attackEdit_->setFixedWidth(60);
    compForm->addRow(tr("Attack (ms):"), attackEdit_);

    releaseEdit_ = new QLineEdit("100.0", this);
    releaseEdit_->setToolTip(tr("Release time in milliseconds"));
    releaseEdit_->setFixedWidth(60);
    compForm->addRow(tr("Release (ms):"), releaseEdit_);

    makeupEdit_ = new QLineEdit("0.0", this);
    makeupEdit_->setToolTip(tr("Makeup gain in dB to compensate for compression"));
    makeupEdit_->setFixedWidth(60);
    compForm->addRow(tr("Makeup (dB):"), makeupEdit_);

    mainLayout->addLayout(compForm);

    // Compress buttons
    auto* compBtnLayout = new QHBoxLayout();
    compBtnLayout->setContentsMargins(10, 0, 0, 0);
    compressSelectedBtn_ = new QPushButton(tr("Compress Selected"), this);
    compressSelectedBtn_->setToolTip(tr("Apply compression to selected clips"));
    compBtnLayout->addWidget(compressSelectedBtn_);
    compressAllBtn_ = new QPushButton(tr("Compress All"), this);
    compressAllBtn_->setToolTip(tr("Apply compression to all clips"));
    compBtnLayout->addWidget(compressAllBtn_);
    compBtnLayout->addStretch();
    mainLayout->addLayout(compBtnLayout);

    // --- Connections ---
    connect(normalizeSelectedBtn_, &QPushButton::clicked, this, &ProcessingPanel::normalizeSelectedRequested);
    connect(normalizeAllBtn_, &QPushButton::clicked, this, &ProcessingPanel::normalizeAllRequested);
    connect(compressSelectedBtn_, &QPushButton::clicked, this, &ProcessingPanel::compressSelectedRequested);
    connect(compressAllBtn_, &QPushButton::clicked, this, &ProcessingPanel::compressAllRequested);
}

double ProcessingPanel::normalizeTarget() const {
    bool ok = false;
    double val = normalizeTargetEdit_->text().toDouble(&ok);
    return ok ? val : -1.0;
}

float ProcessingPanel::compThreshold() const {
    bool ok = false;
    float val = thresholdEdit_->text().toFloat(&ok);
    return ok ? val : -12.0f;
}

float ProcessingPanel::compRatio() const {
    bool ok = false;
    float val = ratioEdit_->text().toFloat(&ok);
    return ok ? val : 4.0f;
}

float ProcessingPanel::compAttackMs() const {
    bool ok = false;
    float val = attackEdit_->text().toFloat(&ok);
    return ok ? val : 10.0f;
}

float ProcessingPanel::compReleaseMs() const {
    bool ok = false;
    float val = releaseEdit_->text().toFloat(&ok);
    return ok ? val : 100.0f;
}

float ProcessingPanel::compMakeupDb() const {
    bool ok = false;
    float val = makeupEdit_->text().toFloat(&ok);
    return ok ? val : 0.0f;
}

