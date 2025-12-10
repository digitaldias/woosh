/**
 * @file SettingsDialog.cpp
 * @brief Implementation of the Settings dialog.
 */

#include "SettingsDialog.h"

#include <QCheckBox>
#include <QDialogButtonBox>
#include <QFormLayout>
#include <QGroupBox>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QVBoxLayout>

SettingsDialog::SettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Settings"));
    setMinimumWidth(350);
    setupUi();
}

void SettingsDialog::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(16);

    // --- User Interface section ---
    auto* uiGroup = new QGroupBox(tr("User Interface"), this);
    auto* uiLayout = new QVBoxLayout(uiGroup);

    tooltipsCheck_ = new QCheckBox(tr("Show column header tooltips"), this);
    tooltipsCheck_->setToolTip(tr("Display explanatory tooltips when hovering over table column headers"));
    tooltipsCheck_->setChecked(true);
    uiLayout->addWidget(tooltipsCheck_);

    mainLayout->addWidget(uiGroup);

    // --- History section ---
    auto* historyGroup = new QGroupBox(tr("History"), this);
    auto* historyLayout = new QVBoxLayout(historyGroup);

    auto* historyLabel = new QLabel(tr("Clear the list of recently opened files and folders."), this);
    historyLabel->setWordWrap(true);
    historyLayout->addWidget(historyLabel);

    clearHistoryBtn_ = new QPushButton(tr("Clear Recent History"), this);
    clearHistoryBtn_->setIcon(QIcon::fromTheme("edit-clear"));
    connect(clearHistoryBtn_, &QPushButton::clicked, this, &SettingsDialog::onClearHistoryClicked);
    historyLayout->addWidget(clearHistoryBtn_);

    mainLayout->addWidget(historyGroup);

    // Stretch
    mainLayout->addStretch();

    // --- Dialog buttons ---
    auto* buttonBox = new QDialogButtonBox(
        QDialogButtonBox::Ok | QDialogButtonBox::Cancel, this);
    connect(buttonBox, &QDialogButtonBox::accepted, this, &QDialog::accept);
    connect(buttonBox, &QDialogButtonBox::rejected, this, &QDialog::reject);
    mainLayout->addWidget(buttonBox);
}

bool SettingsDialog::showColumnTooltips() const {
    return tooltipsCheck_->isChecked();
}

void SettingsDialog::setShowColumnTooltips(bool show) {
    tooltipsCheck_->setChecked(show);
}

void SettingsDialog::onClearHistoryClicked() {
    auto result = QMessageBox::question(
        this,
        tr("Clear History"),
        tr("Are you sure you want to clear the recent files and folders history?"),
        QMessageBox::Yes | QMessageBox::No,
        QMessageBox::No
    );

    if (result == QMessageBox::Yes) {
        Q_EMIT clearHistoryRequested();
        QMessageBox::information(this, tr("History Cleared"),
            tr("Recent files and folders history has been cleared."));
    }
}

