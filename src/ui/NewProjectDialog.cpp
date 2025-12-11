/**
 * @file NewProjectDialog.cpp
 * @brief Dialog for creating a new Woosh project.
 */

#include "NewProjectDialog.h"

#include <QDir>
#include <QVBoxLayout>
#include <QHBoxLayout>
#include <QFormLayout>
#include <QLineEdit>
#include <QPushButton>
#include <QLabel>
#include <QFileDialog>
#include <QMessageBox>

NewProjectDialog::NewProjectDialog(QWidget* parent)
    : QDialog(parent)
{
    setWindowTitle(tr("New Project"));
    setMinimumWidth(500);
    setModal(true);
    setupUi();
}

void NewProjectDialog::setupUi()
{
    auto* mainLayout = new QVBoxLayout(this);
    mainLayout->setSpacing(12);
    mainLayout->setContentsMargins(20, 20, 20, 20);

    // Header
    auto* headerLabel = new QLabel(tr("Create a new Woosh project"), this);
    QFont headerFont = headerLabel->font();
    headerFont.setPointSize(headerFont.pointSize() + 2);
    headerFont.setBold(true);
    headerLabel->setFont(headerFont);
    mainLayout->addWidget(headerLabel);

    auto* descLabel = new QLabel(
        tr("A project tracks your audio files, processing state, and export settings."),
        this
    );
    descLabel->setWordWrap(true);
    descLabel->setStyleSheet("color: gray;");
    mainLayout->addWidget(descLabel);

    mainLayout->addSpacing(10);

    // Form layout
    auto* formLayout = new QFormLayout();
    formLayout->setSpacing(10);

    // Project name
    nameEdit_ = new QLineEdit(this);
    nameEdit_->setPlaceholderText(tr("My Game Audio"));
    formLayout->addRow(tr("Project Name:"), nameEdit_);

    // Game name (for metadata)
    gameNameEdit_ = new QLineEdit(this);
    gameNameEdit_->setPlaceholderText(tr("Game title for MP3 metadata"));
    gameNameEdit_->setToolTip(tr("Used as album name in MP3 tags"));
    formLayout->addRow(tr("Game Name:"), gameNameEdit_);

    // Author name (for metadata)
    authorNameEdit_ = new QLineEdit(this);
    authorNameEdit_->setPlaceholderText(tr("Your name or studio"));
    authorNameEdit_->setToolTip(tr("Used as artist name in MP3 tags"));
    formLayout->addRow(tr("Author:"), authorNameEdit_);

    formLayout->addRow("", new QLabel("", this));  // Spacer

    // RAW folder
    auto* rawLayout = new QHBoxLayout();
    rawFolderEdit_ = new QLineEdit(this);
    rawFolderEdit_->setPlaceholderText(tr("Source audio files folder"));
    rawFolderEdit_->setReadOnly(true);
    browseRawBtn_ = new QPushButton(tr("Browse..."), this);
    browseRawBtn_->setFixedWidth(100);
    rawLayout->addWidget(rawFolderEdit_);
    rawLayout->addWidget(browseRawBtn_);
    formLayout->addRow(tr("RAW Folder:"), rawLayout);

    // RAW folder description
    auto* rawDesc = new QLabel(tr("Contains original, unprocessed audio files"), this);
    rawDesc->setStyleSheet("color: gray; font-size: 11px;");
    formLayout->addRow("", rawDesc);

    // Game folder
    auto* gameLayout = new QHBoxLayout();
    gameFolderEdit_ = new QLineEdit(this);
    gameFolderEdit_->setPlaceholderText(tr("Processed audio output folder"));
    gameFolderEdit_->setReadOnly(true);
    browseGameBtn_ = new QPushButton(tr("Browse..."), this);
    browseGameBtn_->setFixedWidth(100);
    gameLayout->addWidget(gameFolderEdit_);
    gameLayout->addWidget(browseGameBtn_);
    formLayout->addRow(tr("Game Folder:"), gameLayout);

    // Game folder description
    auto* gameDesc = new QLabel(tr("Where processed files will be exported"), this);
    gameDesc->setStyleSheet("color: gray; font-size: 11px;");
    formLayout->addRow("", gameDesc);

    mainLayout->addLayout(formLayout);
    mainLayout->addStretch();

    // Buttons
    auto* buttonLayout = new QHBoxLayout();
    buttonLayout->addStretch();
    
    cancelBtn_ = new QPushButton(tr("Cancel"), this);
    createBtn_ = new QPushButton(tr("Create Project"), this);
    createBtn_->setDefault(true);
    createBtn_->setEnabled(false);
    
    buttonLayout->addWidget(cancelBtn_);
    buttonLayout->addWidget(createBtn_);
    mainLayout->addLayout(buttonLayout);

    // Connections
    connect(browseRawBtn_, &QPushButton::clicked, this, &NewProjectDialog::browseRawFolder);
    connect(browseGameBtn_, &QPushButton::clicked, this, &NewProjectDialog::browseGameFolder);
    connect(nameEdit_, &QLineEdit::textChanged, this, &NewProjectDialog::validateInputs);
    connect(cancelBtn_, &QPushButton::clicked, this, &QDialog::reject);
    connect(createBtn_, &QPushButton::clicked, this, &QDialog::accept);
}

void NewProjectDialog::browseRawFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select RAW Audio Folder"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        rawFolderEdit_->setText(dir);
        validateInputs();
    }
}

void NewProjectDialog::browseGameFolder()
{
    QString dir = QFileDialog::getExistingDirectory(
        this,
        tr("Select Game Audio Folder"),
        QString(),
        QFileDialog::ShowDirsOnly | QFileDialog::DontResolveSymlinks
    );
    
    if (!dir.isEmpty()) {
        gameFolderEdit_->setText(dir);
        validateInputs();
    }
}

void NewProjectDialog::validateInputs()
{
    QString rawFolder = rawFolderEdit_->text();
    QString gameFolder = gameFolderEdit_->text();
    
    bool valid = !nameEdit_->text().trimmed().isEmpty() &&
                 !rawFolder.isEmpty() &&
                 !gameFolder.isEmpty() &&
                 QDir(rawFolder).canonicalPath() != QDir(gameFolder).canonicalPath();
    
    createBtn_->setEnabled(valid);
    
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

QString NewProjectDialog::projectName() const
{
    return nameEdit_->text().trimmed();
}

QString NewProjectDialog::gameName() const
{
    return gameNameEdit_->text().trimmed();
}

QString NewProjectDialog::authorName() const
{
    return authorNameEdit_->text().trimmed();
}

void NewProjectDialog::setDefaultAuthorName(const QString& name)
{
    authorNameEdit_->setText(name);
}

QString NewProjectDialog::rawFolder() const
{
    return rawFolderEdit_->text();
}

QString NewProjectDialog::gameFolder() const
{
    return gameFolderEdit_->text();
}
