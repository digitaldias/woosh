/**
 * @file OutputPanel.cpp
 * @brief Implementation of the OutputPanel widget.
 */

#include "OutputPanel.h"

#include <QCheckBox>
#include <QFileDialog>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QPushButton>
#include <QVBoxLayout>

OutputPanel::OutputPanel(QWidget* parent)
    : QGroupBox(tr("Output"), parent)
{
    setupUi();
}

void OutputPanel::setupUi() {
    auto* mainLayout = new QVBoxLayout(this);

    // --- Output folder row ---
    auto* folderLayout = new QHBoxLayout();
    folderLayout->addWidget(new QLabel(tr("Output Folder:"), this));

    folderEdit_ = new QLineEdit(this);
    folderEdit_->setPlaceholderText(tr("Select output folder..."));
    folderEdit_->setReadOnly(true);
    folderEdit_->setToolTip(tr("Folder where processed files will be saved"));
    folderLayout->addWidget(folderEdit_, 1);

    browseBtn_ = new QPushButton(tr("..."), this);
    browseBtn_->setFixedWidth(30);
    browseBtn_->setToolTip(tr("Browse for output folder"));
    folderLayout->addWidget(browseBtn_);

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
    connect(browseBtn_, &QPushButton::clicked, this, &OutputPanel::onBrowseClicked);
    connect(exportSelectedBtn_, &QPushButton::clicked, this, &OutputPanel::exportSelected);
    connect(exportAllBtn_, &QPushButton::clicked, this, &OutputPanel::exportAll);
}

QString OutputPanel::outputFolder() const {
    return folderEdit_->text();
}

void OutputPanel::setOutputFolder(const QString& path) {
    folderEdit_->setText(path);
}

bool OutputPanel::overwriteOriginals() const {
    return overwriteCheck_->isChecked();
}

void OutputPanel::onBrowseClicked() {
    QString startDir = folderEdit_->text();
    if (startDir.isEmpty()) {
        startDir = QDir::homePath();
    }

    QString folder = QFileDialog::getExistingDirectory(
        this,
        tr("Select Output Folder"),
        startDir
    );

    if (!folder.isEmpty()) {
        folderEdit_->setText(folder);
        Q_EMIT outputFolderChanged(folder);
    }
}

