/**
 * @file ProjectSettingsDialog.cpp
 * @brief Dialog for editing Woosh project settings.
 */

#include "ProjectSettingsDialog.h"

#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QComboBox>
#include <QSpinBox>
#include <QPushButton>
#include <QLabel>
#include <QTabWidget>
#include <QFileDialog>
#include <QGroupBox>

ProjectSettingsDialog::ProjectSettingsDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("Project Settings"));
    setMinimumSize(550, 450);
    setModal(true);
    setupUi();
}

void ProjectSettingsDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(16, 16, 16, 16);

    // Tabs
    auto* tabs = new QTabWidget(this);
    setupGeneralTab(tabs);
    setupExportTab(tabs);
    setupMetadataTab(tabs);
    mainLayout->addWidget(tabs);

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    cancelBtn_ = new QPushButton(tr("Cancel"), this);
    okBtn_ = new QPushButton(tr("OK"), this);
    okBtn_->setDefault(true);
    
    buttonLayout->addWidget(cancelBtn_);
    buttonLayout->addWidget(okBtn_);
    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(cancelBtn_, &QPushButton::clicked, this, &QDialog::reject);
    connect(okBtn_, &QPushButton::clicked, this, &QDialog::accept);
}

void ProjectSettingsDialog::setupGeneralTab(QTabWidget* tabs)
{
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    // Project name
    nameEdit_ = new QLineEdit(widget);
    formLayout->addRow(tr("Project Name:"), nameEdit_);

    // RAW folder
    auto* rawLayout = new QHBoxLayout();
    rawFolderEdit_ = new QLineEdit(widget);
    rawFolderEdit_->setReadOnly(true);
    browseRawBtn_ = new QPushButton(tr("Browse..."), widget);
    browseRawBtn_->setFixedWidth(100);
    rawLayout->addWidget(rawFolderEdit_);
    rawLayout->addWidget(browseRawBtn_);
    formLayout->addRow(tr("RAW Folder:"), rawLayout);

    auto* rawDesc = new QLabel(tr("Source folder containing original audio files"), widget);
    rawDesc->setStyleSheet("color: gray; font-size: 11px;");
    formLayout->addRow("", rawDesc);

    // Game folder
    auto* gameLayout = new QHBoxLayout();
    gameFolderEdit_ = new QLineEdit(widget);
    gameFolderEdit_->setReadOnly(true);
    browseGameBtn_ = new QPushButton(tr("Browse..."), widget);
    browseGameBtn_->setFixedWidth(100);
    gameLayout->addWidget(gameFolderEdit_);
    gameLayout->addWidget(browseGameBtn_);
    formLayout->addRow(tr("Game Folder:"), gameLayout);

    auto* gameDesc = new QLabel(tr("Destination folder for processed audio files"), widget);
    gameDesc->setStyleSheet("color: gray; font-size: 11px;");
    formLayout->addRow("", gameDesc);

    layout->addLayout(formLayout);
    layout->addStretch();

    // Connections
    connect(browseRawBtn_, &QPushButton::clicked, this, &ProjectSettingsDialog::browseRawFolder);
    connect(browseGameBtn_, &QPushButton::clicked, this, &ProjectSettingsDialog::browseGameFolder);
    connect(nameEdit_, &QLineEdit::textChanged, this, &ProjectSettingsDialog::validateInputs);

    tabs->addTab(widget, tr("General"));
}

void ProjectSettingsDialog::setupExportTab(QTabWidget* tabs)
{
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);

    // Format group
    auto* formatGroup = new QGroupBox(tr("Export Format"), widget);
    auto* formatLayout = new QFormLayout(formatGroup);

    formatCombo_ = new QComboBox(formatGroup);
    formatCombo_->addItem(tr("WAV (Uncompressed)"), static_cast<int>(ExportFormat::WAV));
    formatCombo_->addItem(tr("MP3 (Game-optimized)"), static_cast<int>(ExportFormat::MP3));
    formatCombo_->addItem(tr("OGG Vorbis"), static_cast<int>(ExportFormat::OGG));
    formatLayout->addRow(tr("Format:"), formatCombo_);

    bitrateCombo_ = new QComboBox(formatGroup);
    bitrateCombo_->addItem(tr("128 kbps (Smallest)"), 128);
    bitrateCombo_->addItem(tr("160 kbps (Balanced)"), 160);
    bitrateCombo_->addItem(tr("192 kbps (Best Quality)"), 192);
    bitrateCombo_->setCurrentIndex(1);  // Default to 160
    formatLayout->addRow(tr("Bitrate:"), bitrateCombo_);

    layout->addWidget(formatGroup);

    // Info label
    auto* infoLabel = new QLabel(
        tr("MP3 format is recommended for game audio:\n"
           "• Small file size for faster loading\n"
           "• 160 kbps provides excellent quality for games\n"
           "• Wide compatibility across platforms"),
        widget
    );
    infoLabel->setStyleSheet("color: gray; padding: 10px;");
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    layout->addStretch();

    // Connections
    connect(formatCombo_, QOverload<int>::of(&QComboBox::currentIndexChanged),
            this, &ProjectSettingsDialog::onFormatChanged);

    tabs->addTab(widget, tr("Export"));
}

void ProjectSettingsDialog::setupMetadataTab(QTabWidget* tabs)
{
    auto* widget = new QWidget(this);
    auto* layout = new QVBoxLayout(widget);
    layout->setContentsMargins(16, 16, 16, 16);

    auto* metaGroup = new QGroupBox(tr("ID3 Metadata (for MP3/OGG)"), widget);
    auto* metaLayout = new QFormLayout(metaGroup);

    artistEdit_ = new QLineEdit(metaGroup);
    artistEdit_->setPlaceholderText(tr("Your name or studio name"));
    metaLayout->addRow(tr("Artist:"), artistEdit_);

    albumEdit_ = new QLineEdit(metaGroup);
    albumEdit_->setPlaceholderText(tr("Game or project name"));
    metaLayout->addRow(tr("Album/Game:"), albumEdit_);

    commentEdit_ = new QLineEdit(metaGroup);
    commentEdit_->setText(tr("Made by Woosh"));
    metaLayout->addRow(tr("Comment:"), commentEdit_);

    layout->addWidget(metaGroup);

    auto* infoLabel = new QLabel(
        tr("Metadata is embedded in MP3 and OGG files.\n"
           "This helps identify your audio assets."),
        widget
    );
    infoLabel->setStyleSheet("color: gray; padding: 10px;");
    infoLabel->setWordWrap(true);
    layout->addWidget(infoLabel);

    layout->addStretch();

    tabs->addTab(widget, tr("Metadata"));
}

void ProjectSettingsDialog::browseRawFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select RAW Audio Folder"),
        rawFolderEdit_->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        rawFolderEdit_->setText(dir);
        validateInputs();
    }
}

void ProjectSettingsDialog::browseGameFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select Game Audio Folder"),
        gameFolderEdit_->text(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        gameFolderEdit_->setText(dir);
        validateInputs();
    }
}

void ProjectSettingsDialog::onFormatChanged(int index)
{
    // Enable bitrate only for lossy formats
    auto format = static_cast<ExportFormat>(formatCombo_->itemData(index).toInt());
    bitrateCombo_->setEnabled(format != ExportFormat::WAV);
}

void ProjectSettingsDialog::validateInputs()
{
    QString rawFolder = rawFolderEdit_->text();
    QString gameFolder = gameFolderEdit_->text();
    
    bool valid = !nameEdit_->text().trimmed().isEmpty() &&
                 !rawFolder.isEmpty() &&
                 !gameFolder.isEmpty() &&
                 QDir(rawFolder).canonicalPath() != QDir(gameFolder).canonicalPath();
    
    okBtn_->setEnabled(valid);
    
    // Show warning if folders are the same
    if (!rawFolder.isEmpty() && !gameFolder.isEmpty() &&
        QDir(rawFolder).canonicalPath() == QDir(gameFolder).canonicalPath()) {
        gameFolderEdit_->setStyleSheet("border: 1px solid red;");
        gameFolderEdit_->setToolTip(tr("Output folder must be different from input folder"));
    } else {
        gameFolderEdit_->setStyleSheet("");
        gameFolderEdit_->setToolTip("");
    }
}

void ProjectSettingsDialog::loadFromProject(const Project& project)
{
    nameEdit_->setText(QString::fromStdString(project.name()));
    rawFolderEdit_->setText(QString::fromStdString(project.rawFolder()));
    gameFolderEdit_->setText(QString::fromStdString(project.gameFolder()));

    // Export settings
    auto exportSettings = project.exportSettings();
    int formatIndex = formatCombo_->findData(static_cast<int>(exportSettings.format));
    if (formatIndex >= 0) {
        formatCombo_->setCurrentIndex(formatIndex);
    }
    
    int bitrateIndex = bitrateCombo_->findData(exportSettings.mp3Bitrate);
    if (bitrateIndex >= 0) {
        bitrateCombo_->setCurrentIndex(bitrateIndex);
    }

    artistEdit_->setText(QString::fromStdString(exportSettings.authorName));
    albumEdit_->setText(QString::fromStdString(exportSettings.gameName));
    // Comment not stored in ExportSettings, use a default
    commentEdit_->setText(tr("Made by Woosh"));

    onFormatChanged(formatCombo_->currentIndex());
    validateInputs();
}

void ProjectSettingsDialog::applyToProject(Project& project) const
{
    project.setName(nameEdit_->text().trimmed().toStdString());
    project.setRawFolder(rawFolderEdit_->text().toStdString());
    project.setGameFolder(gameFolderEdit_->text().toStdString());

    ExportSettings exportSettings;
    exportSettings.format = static_cast<ExportFormat>(
        formatCombo_->currentData().toInt()
    );
    exportSettings.mp3Bitrate = bitrateCombo_->currentData().toInt();
    exportSettings.authorName = artistEdit_->text().toStdString();
    exportSettings.gameName = albumEdit_->text().toStdString();
    exportSettings.embedMetadata = true;
    
    project.setExportSettings(exportSettings);
}

QString ProjectSettingsDialog::projectName() const
{
    return nameEdit_->text().trimmed();
}

QString ProjectSettingsDialog::rawFolder() const
{
    return rawFolderEdit_->text();
}

QString ProjectSettingsDialog::gameFolder() const
{
    return gameFolderEdit_->text();
}

ExportFormat ProjectSettingsDialog::exportFormat() const
{
    return static_cast<ExportFormat>(formatCombo_->currentData().toInt());
}

int ProjectSettingsDialog::bitrate() const
{
    return bitrateCombo_->currentData().toInt();
}

QString ProjectSettingsDialog::metadataArtist() const
{
    return artistEdit_->text();
}

QString ProjectSettingsDialog::metadataAlbum() const
{
    return albumEdit_->text();
}

QString ProjectSettingsDialog::metadataComment() const
{
    return commentEdit_->text();
}
