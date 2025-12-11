/**
 * @file OutputPanel.cpp
 * @brief Implementation of the OutputPanel widget.
 */

#include "OutputPanel.h"

#include <QCheckBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPushButton>
#include <QVBoxLayout>

OutputPanel::OutputPanel(QWidget* parent)
    : QGroupBox(tr("Output"), parent)
{
    setupUi();
}

void OutputPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // --- Output folder row (read-only display) ---
    auto* folderLayout = new QHBoxLayout();
    folderLayout->addWidget(new QLabel(tr("Output Folder:"), this));

    folderLabel_ = new QLabel(this);
    folderLabel_->setTextInteractionFlags(Qt::TextSelectableByMouse);
    folderLabel_->setStyleSheet("QLabel { color: #888; padding: 2px; }");
    folderLabel_->setToolTip(tr("Folder where processed files will be saved (configured in project settings)"));
    folderLayout->addWidget(folderLabel_, 1);

    mainLayout->addLayout(folderLayout);

    // --- Overwrite checkbox ---
    overwriteCheck_ = new QCheckBox(tr("Overwrite original files"), this);
    overwriteCheck_->setChecked(false);  // Default OFF for safety
    overwriteCheck_->setToolTip(tr("WARNING: If checked, original files will be replaced!"));
    mainLayout->addWidget(overwriteCheck_);

    // --- Export buttons ---
    mainLayout->addSpacing(10);

    auto* btnLayout = new QHBoxLayout();
    exportSelectedBtn_ = new QPushButton(tr("Export Selected"), this);
    exportSelectedBtn_->setToolTip(tr("Export only the selected clips"));
    btnLayout->addWidget(exportSelectedBtn_);

    exportAllBtn_ = new QPushButton(tr("Export All"), this);
    exportAllBtn_->setToolTip(tr("Export all loaded clips"));
    btnLayout->addWidget(exportAllBtn_);

    mainLayout->addLayout(btnLayout);

    // --- Connections ---
    connect(exportSelectedBtn_, &QPushButton::clicked, this, &OutputPanel::exportSelected);
    connect(exportAllBtn_, &QPushButton::clicked, this, &OutputPanel::exportAll);
}

QString OutputPanel::outputFolder() const {
    return outputFolder_;
}

void OutputPanel::setOutputFolder(const QString& path) {
    outputFolder_ = path;
    if (path.isEmpty()) {
        folderLabel_->setText(tr("(Set in Project → New/Edit Project)"));
    } else {
        // Show abbreviated path if too long
        QFontMetrics fm(folderLabel_->font());
        QString elidedPath = fm.elidedText(path, Qt::ElideMiddle, 300);
        folderLabel_->setText(elidedPath);
        folderLabel_->setToolTip(path);  // Full path in tooltip
    }
}

bool OutputPanel::overwriteOriginals() const {
    return overwriteCheck_->isChecked();
}

