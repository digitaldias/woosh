/**
 * @file MainWindow.cpp
 * @brief Implementation of the main application window.
 */

#include "MainWindow.h"

#include <QAction>
#include <QCloseEvent>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
#include <QHBoxLayout>
#include <QHeaderView>
#include <QItemSelection>
#include <QKeyEvent>
#include <QLabel>
#include <QMenu>
#include <QMenuBar>
#include <QMessageBox>
#include <QPixmap>
#include <QProgressBar>
#include <QPushButton>
#include <QSettings>
#include <QSortFilterProxyModel>
#include <QSplitter>
#include <QStatusBar>
#include <QTableView>
#include <QVBoxLayout>

#include <QtConcurrent>

#include "audio/AudioPlayer.h"
#include "audio/Formats/Mp3Encoder.h"
#include "ui/ClipTableModel.h"
#include "ui/OutputPanel.h"
#include "ui/ProcessingPanel.h"
#include "ui/SettingsDialog.h"
#include "ui/TransportPanel.h"
#include "ui/WaveformView.h"
#include "ui/VuMeterWidget.h"
#include "ui/NewProjectDialog.h"
#include "ui/ProjectSettingsDialog.h"
#include "utils/FileScanner.h"

// Application settings keys
namespace {
constexpr const char* kSettingsOrg = "Woosh";
constexpr const char* kSettingsApp = "WooshEditor";
constexpr const char* kKeyLastDir = "LastOpenDirectory";
constexpr const char* kKeyOutputDir = "OutputDirectory";
constexpr const char* kKeyWindowGeom = "WindowGeometry";
constexpr const char* kKeyRecentFiles = "RecentFiles";
constexpr const char* kKeyRecentFolders = "RecentFolders";
constexpr const char* kKeyDefaultAuthor = "DefaultAuthorName";
constexpr const char* kKeyShowTooltips = "ShowColumnTooltips";
}  // namespace

// ============================================================================
// Construction / Destruction
// ============================================================================

MainWindow::MainWindow(QWidget* parent)
    : QMainWindow(parent)
    , settings_(std::make_unique<QSettings>(kSettingsOrg, kSettingsApp))
{
    loadSettings();
    setupUi();
    setupMenus();
    
    // Connect ProjectManager signals
    connect(&projectManager_, &ProjectManager::projectChanged, 
            this, &MainWindow::onProjectChanged);
    connect(&projectManager_, &ProjectManager::dirtyStateChanged, 
            this, &MainWindow::onDirtyStateChanged);
    connect(&projectManager_, &ProjectManager::recentProjectsChanged,
            this, &MainWindow::onRecentProjectsChanged);
    
    updateWindowTitle();
    statusBar()->showMessage(tr("Ready"));
}

MainWindow::~MainWindow() {
    // Stop and disconnect audio player before child widgets are destroyed
    if (audioPlayer_) {
        audioPlayer_->stop();
        audioPlayer_->disconnect();
    }

    saveSettings();
}

void MainWindow::loadSettings() {
    lastOpenDirectory_ = settings_->value(kKeyLastDir, QDir::homePath()).toString();
    recentFiles_ = settings_->value(kKeyRecentFiles).toStringList();
    recentFolders_ = settings_->value(kKeyRecentFolders).toStringList();
    defaultAuthorName_ = settings_->value(kKeyDefaultAuthor).toString();
    showColumnTooltips_ = settings_->value(kKeyShowTooltips, true).toBool();
}

void MainWindow::saveSettings() {
    settings_->setValue(kKeyLastDir, lastOpenDirectory_);
    settings_->setValue(kKeyRecentFiles, recentFiles_);
    settings_->setValue(kKeyRecentFolders, recentFolders_);
    settings_->setValue(kKeyDefaultAuthor, defaultAuthorName_);
    settings_->setValue(kKeyShowTooltips, showColumnTooltips_);
    if (outputPanel_) {
        settings_->setValue(kKeyOutputDir, outputPanel_->outputFolder());
    }
    settings_->setValue(kKeyWindowGeom, saveGeometry());
}

// ============================================================================
// UI Setup
// ============================================================================

void MainWindow::setupUi() {
    setWindowTitle(tr("Woosh - Audio Batch Editor"));
    resize(1100, 700);

    auto* central = new QWidget(this);
    auto* mainLayout = new QVBoxLayout(central);

    auto* splitter = new QSplitter(Qt::Horizontal, central);

    // Left pane: Clip table
    clipModel_ = new ClipTableModel(clips_, projectManager_, this);
    proxyModel_ = new QSortFilterProxyModel(this);
    proxyModel_->setSourceModel(clipModel_);
    proxyModel_->setSortRole(Qt::UserRole);

    clipTable_ = new QTableView(splitter);
    clipTable_->setModel(proxyModel_);
    clipTable_->setSortingEnabled(true);
    clipTable_->setSelectionBehavior(QAbstractItemView::SelectRows);
    clipTable_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    clipTable_->setAlternatingRowColors(true);
    clipTable_->horizontalHeader()->setStretchLastSection(true);
    clipTable_->horizontalHeader()->setSectionResizeMode(QHeaderView::ResizeToContents);
    clipTable_->verticalHeader()->setVisible(false);
    clipTable_->sortByColumn(0, Qt::AscendingOrder);

    connect(clipTable_->selectionModel(), &QItemSelectionModel::selectionChanged,
            this, &MainWindow::onSelectionChanged);

    // Right pane
    auto* rightPane = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPane);
    rightLayout->setContentsMargins(0, 0, 0, 0);
    rightLayout->setSpacing(4);

    waveformView_ = new WaveformView(rightPane);
    waveformView_->setMinimumHeight(200);
    rightLayout->addWidget(waveformView_, 1);

    connect(waveformView_, &WaveformView::seekRequested, this, &MainWindow::onSeek);
    connect(waveformView_, &WaveformView::trimChanged, this, &MainWindow::onTrimChanged);

    transportPanel_ = new TransportPanel(rightPane);
    rightLayout->addWidget(transportPanel_);

    connect(transportPanel_, &TransportPanel::playPauseClicked, this, &MainWindow::onPlayPause);
    connect(transportPanel_, &TransportPanel::stopClicked, this, &MainWindow::onStop);
    connect(transportPanel_, &TransportPanel::zoomInClicked, waveformView_, &WaveformView::zoomIn);
    connect(transportPanel_, &TransportPanel::zoomOutClicked, waveformView_, &WaveformView::zoomOut);
    connect(transportPanel_, &TransportPanel::zoomFitClicked, waveformView_, &WaveformView::zoomToFit);
    connect(transportPanel_, &TransportPanel::applyTrimClicked, this, &MainWindow::onApplyTrim);

    // Processing + VU meter side by side
    auto* processingRow = new QWidget(rightPane);
    auto* processingRowLayout = new QHBoxLayout(processingRow);
    processingRowLayout->setContentsMargins(0, 0, 0, 0);
    processingRowLayout->setSpacing(4);

    processingPanel_ = new ProcessingPanel(processingRow);
    processingRowLayout->addWidget(processingPanel_, 1);

    vuMeter_ = new VuMeterWidget(processingRow);
    processingRowLayout->addWidget(vuMeter_, 0);

    rightLayout->addWidget(processingRow);

    connect(processingPanel_, &ProcessingPanel::normalizeSelectedRequested, this, &MainWindow::onNormalizeSelected);
    connect(processingPanel_, &ProcessingPanel::normalizeAllRequested, this, &MainWindow::onNormalizeAll);
    connect(processingPanel_, &ProcessingPanel::compressSelectedRequested, this, &MainWindow::onCompressSelected);
    connect(processingPanel_, &ProcessingPanel::compressAllRequested, this, &MainWindow::onCompressAll);

    outputPanel_ = new OutputPanel(rightPane);
    rightLayout->addWidget(outputPanel_);

    QString savedOutput = settings_->value(kKeyOutputDir).toString();
    if (!savedOutput.isEmpty()) {
        outputPanel_->setOutputFolder(savedOutput);
    }

    connect(outputPanel_, &OutputPanel::exportSelected, this, &MainWindow::onExportSelected);
    connect(outputPanel_, &OutputPanel::exportAll, this, &MainWindow::onExportAll);

    rightLayout->addStretch();

    splitter->addWidget(clipTable_);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(0, 2);
    splitter->setStretchFactor(1, 1);

    progressBar_ = new QProgressBar(central);
    progressBar_->setMinimum(0);
    progressBar_->setMaximum(100);
    progressBar_->setValue(0);
    progressBar_->setVisible(false);

    mainLayout->addWidget(splitter);
    mainLayout->addWidget(progressBar_);

    setCentralWidget(central);

    statusLabel_ = new QLabel(this);
    statusBar()->addPermanentWidget(statusLabel_);

    audioPlayer_ = new AudioPlayer(this);
    connect(audioPlayer_, &AudioPlayer::positionChanged, this, &MainWindow::onPlaybackPositionChanged);
    connect(audioPlayer_, &AudioPlayer::finished, this, &MainWindow::onPlaybackFinished);
    connect(audioPlayer_, &AudioPlayer::stateChanged, this, [this](AudioPlayer::State state) {
        transportPanel_->setPlaying(state == AudioPlayer::State::Playing);
    });

    if (vuMeter_) {
        connect(audioPlayer_, &AudioPlayer::levelsChanged,
                vuMeter_, &VuMeterWidget::setLevels);
    }

    QByteArray geom = settings_->value(kKeyWindowGeom).toByteArray();
    if (!geom.isEmpty()) {
        restoreGeometry(geom);
    }
}

void MainWindow::setupMenus() {
    // --- File menu ---
    auto* fileMenu = menuBar()->addMenu(tr("&File"));

    // Project actions
    newProjectAction_ = fileMenu->addAction(tr("&New Project..."), this, &MainWindow::newProject);
    newProjectAction_->setShortcut(QKeySequence::New);

    openProjectAction_ = fileMenu->addAction(tr("&Open Project..."), this, &MainWindow::openProject);
    openProjectAction_->setShortcut(QKeySequence::Open);

    saveProjectAction_ = fileMenu->addAction(tr("&Save Project"), this, &MainWindow::saveProject);
    saveProjectAction_->setShortcut(QKeySequence::Save);

    saveProjectAsAction_ = fileMenu->addAction(tr("Save Project &As..."), this, &MainWindow::saveProjectAs);
    saveProjectAsAction_->setShortcut(QKeySequence::SaveAs);

    fileMenu->addSeparator();

    // Recent Projects submenu
    recentProjectsMenu_ = fileMenu->addMenu(tr("Recent &Projects"));
    updateRecentProjectsMenu();

    fileMenu->addSeparator();

    projectSettingsAction_ = fileMenu->addAction(tr("Project Settin&gs..."), this, &MainWindow::openProjectSettings);

    fileMenu->addSeparator();

    // Legacy file operations (hidden submenu for power users)
    auto* legacyMenu = fileMenu->addMenu(tr("&Import"));
    openFilesAction_ = legacyMenu->addAction(tr("Import &Files..."), this, &MainWindow::openFiles);
    openFolderAction_ = legacyMenu->addAction(tr("Import F&older..."), this, &MainWindow::openFolder);

    // Recent Files submenu (moved to legacy)
    recentFilesMenu_ = legacyMenu->addMenu(tr("Recent &Files"));
    updateRecentFilesMenu();

    // Recent Folders submenu (moved to legacy)
    recentFoldersMenu_ = legacyMenu->addMenu(tr("Recent F&olders"));
    updateRecentFoldersMenu();

    fileMenu->addSeparator();

    settingsAction_ = fileMenu->addAction(tr("&Settings..."), this, &MainWindow::openSettings);
    settingsAction_->setShortcut(QKeySequence::Preferences);

    fileMenu->addSeparator();

    exitAction_ = fileMenu->addAction(tr("E&xit"), this, &QMainWindow::close);
    exitAction_->setShortcut(QKeySequence::Quit);

    // --- Edit menu ---
    auto* editMenu = menuBar()->addMenu(tr("&Edit"));

    undoAction_ = editMenu->addAction(tr("&Undo Processing"), this, &MainWindow::onUndoProcessing);
    undoAction_->setShortcut(QKeySequence::Undo);
    undoAction_->setEnabled(false);

    // --- Help menu ---
    auto* helpMenu = menuBar()->addMenu(tr("&Help"));
    helpMenu->addAction(tr("&About"), this, [this]() {
        QDialog aboutDialog(this);
        aboutDialog.setWindowTitle(tr("About Woosh"));
        aboutDialog.setFixedSize(420, 380);

        auto* layout = new QVBoxLayout(&aboutDialog);

        auto* logoLabel = new QLabel(&aboutDialog);
        QPixmap logo(":/images/logo.png");
        if (!logo.isNull()) {
            logoLabel->setPixmap(logo.scaled(300, 120, Qt::KeepAspectRatio, Qt::SmoothTransformation));
        }
        logoLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(logoLabel);

        auto* titleLabel = new QLabel("<h2>Woosh v0.1.0</h2>", &aboutDialog);
        titleLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(titleLabel);

        auto* descLabel = new QLabel(
            "<p style='text-align:center;'>"
            "<b>The audio batch editor that goes WOOSH!</b><br><br>"
            "Trim, normalize, compress — all your game audio<br>"
            "in one mighty sweep. MP3 export coming soon™.<br><br>"
            "<i>Created by <b>digitaldias</b></i><br>"
            "Cloud whisperer by day, code alchemist by night.<br>"
            "Has mass confused AI systems into submission,<br>"
            "runs on coffee and mass curiosity, and once<br>"
            "convinced a neural network he was a friendly toaster.<br><br>"
            "© 2025 — Made with questionable sanity in Norway 🇳🇴"
            "</p>",
            &aboutDialog);
        descLabel->setWordWrap(true);
        descLabel->setAlignment(Qt::AlignCenter);
        layout->addWidget(descLabel);

        layout->addStretch();

        auto* okBtn = new QPushButton(tr("Woosh!"), &aboutDialog);
        connect(okBtn, &QPushButton::clicked, &aboutDialog, &QDialog::accept);
        layout->addWidget(okBtn, 0, Qt::AlignCenter);

        aboutDialog.exec();
    });
}

// ============================================================================
// Recent files/folders management
// ============================================================================

void MainWindow::addRecentFile(const QString& path) {
    recentFiles_.removeAll(path);
    recentFiles_.prepend(path);
    while (recentFiles_.size() > kMaxRecentItems) {
        recentFiles_.removeLast();
    }
    updateRecentFilesMenu();
}

void MainWindow::addRecentFolder(const QString& path) {
    recentFolders_.removeAll(path);
    recentFolders_.prepend(path);
    while (recentFolders_.size() > kMaxRecentItems) {
        recentFolders_.removeLast();
    }
    updateRecentFoldersMenu();
}

void MainWindow::updateRecentFilesMenu() {
    if (!recentFilesMenu_) return;

    recentFilesMenu_->clear();

    if (recentFiles_.isEmpty()) {
        auto* emptyAction = recentFilesMenu_->addAction(tr("(No recent files)"));
        emptyAction->setEnabled(false);
        return;
    }

    for (const QString& path : recentFiles_) {
        QFileInfo fi(path);
        QString displayName = fi.fileName();
        auto* action = recentFilesMenu_->addAction(displayName);
        action->setData(path);
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFile);
    }
}

void MainWindow::updateRecentFoldersMenu() {
    if (!recentFoldersMenu_) return;

    recentFoldersMenu_->clear();

    if (recentFolders_.isEmpty()) {
        auto* emptyAction = recentFoldersMenu_->addAction(tr("(No recent folders)"));
        emptyAction->setEnabled(false);
        return;
    }

    for (const QString& path : recentFolders_) {
        QFileInfo fi(path);
        QString displayName = fi.fileName();
        if (displayName.isEmpty()) displayName = path;
        auto* action = recentFoldersMenu_->addAction(displayName);
        action->setData(path);
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentFolder);
    }
}

void MainWindow::clearRecentHistory() {
    recentFiles_.clear();
    recentFolders_.clear();
    updateRecentFilesMenu();
    updateRecentFoldersMenu();
}

void MainWindow::openRecentFile() {
    auto* action = qobject_cast<QAction*>(sender());
    if (!action) return;

    QString path = action->data().toString();
    if (path.isEmpty()) return;

    QFileInfo fi(path);
    if (!fi.exists()) {
        QMessageBox::warning(this, tr("File Not Found"),
            tr("The file no longer exists:\n%1").arg(path));
        recentFiles_.removeAll(path);
        updateRecentFilesMenu();
        return;
    }

    lastOpenDirectory_ = fi.absolutePath();
    loadFileList({path});
    addRecentFile(path);
}

void MainWindow::openRecentFolder() {
    auto* action = qobject_cast<QAction*>(sender());
    if (!action) return;

    QString path = action->data().toString();
    if (path.isEmpty()) return;

    QDir dir(path);
    if (!dir.exists()) {
        QMessageBox::warning(this, tr("Folder Not Found"),
            tr("The folder no longer exists:\n%1").arg(path));
        recentFolders_.removeAll(path);
        updateRecentFoldersMenu();
        return;
    }

    lastOpenDirectory_ = path;

    FileScanner scanner;
    auto found = scanner.scan(path.toStdString());

    if (found.empty()) {
        QMessageBox::information(this, tr("No Files Found"),
            tr("No audio files (WAV/MP3) found in the selected folder."));
        return;
    }

    QStringList list;
    list.reserve(static_cast<int>(found.size()));
    for (const auto& p : found) {
        list << QString::fromStdString(p);
    }

    loadFileList(list);
    addRecentFolder(path);
}

// ============================================================================
// Project Management
// ============================================================================

void MainWindow::newProject() {
    if (!maybeSaveProject()) return;

    NewProjectDialog dialog(this);
    dialog.setDefaultAuthorName(defaultAuthorName_);
    if (dialog.exec() != QDialog::Accepted) return;

    projectManager_.newProject(
        dialog.projectName(),
        dialog.rawFolder(),
        dialog.gameFolder()
    );

    // Set export settings with metadata from dialog
    if (projectManager_.hasProject()) {
        ExportSettings exportSettings;
        exportSettings.format = ExportFormat::MP3;
        exportSettings.mp3Bitrate = 160;
        exportSettings.gameName = dialog.gameName().toStdString();
        exportSettings.authorName = dialog.authorName().toStdString();
        exportSettings.embedMetadata = true;
        projectManager_.project().setExportSettings(exportSettings);
    }

    // Clear existing clips and load from RAW folder
    clips_.clear();
    clipModel_->refresh();
    
    loadProjectClips();
    updateWindowTitle();
    statusBar()->showMessage(tr("Created new project: %1").arg(dialog.projectName()));
}

void MainWindow::openProject() {
    if (!maybeSaveProject()) return;

    QString path = QFileDialog::getOpenFileName(
        this, tr("Open Project"),
        lastOpenDirectory_,
        tr("Woosh Projects (*.wooshp)")
    );

    if (path.isEmpty()) return;

    QFileInfo fi(path);
    lastOpenDirectory_ = fi.absolutePath();

    if (!projectManager_.openProject(path)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to open project:\n%1").arg(path));
        return;
    }

    clips_.clear();
    clipModel_->refresh();
    
    loadProjectClips();
    updateWindowTitle();
    statusBar()->showMessage(tr("Opened project: %1").arg(fi.baseName()));
}

void MainWindow::saveProject() {
    if (!projectManager_.hasProject()) {
        QMessageBox::information(this, tr("No Project"),
            tr("No project is currently open. Use File → New Project to create one."));
        return;
    }

    auto& project = projectManager_.project();
    if (project.filePath().empty()) {
        saveProjectAs();
        return;
    }

    if (projectManager_.saveProject()) {
        statusBar()->showMessage(tr("Project saved"));
    } else {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to save project."));
    }
}

void MainWindow::saveProjectAs() {
    if (!projectManager_.hasProject()) {
        QMessageBox::information(this, tr("No Project"),
            tr("No project is currently open. Use File → New Project to create one."));
        return;
    }

    auto& project = projectManager_.project();
    QString path = QFileDialog::getSaveFileName(
        this, tr("Save Project As"),
        lastOpenDirectory_ + "/" + QString::fromStdString(project.name()) + ".wooshp",
        tr("Woosh Projects (*.wooshp)")
    );

    if (path.isEmpty()) return;

    // Ensure .wooshp extension
    if (!path.endsWith(".wooshp", Qt::CaseInsensitive)) {
        path += ".wooshp";
    }

    QFileInfo fi(path);
    lastOpenDirectory_ = fi.absolutePath();

    if (projectManager_.saveProjectAs(path)) {
        updateWindowTitle();
        statusBar()->showMessage(tr("Project saved as: %1").arg(fi.fileName()));
    } else {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to save project."));
    }
}

void MainWindow::openRecentProject() {
    auto* action = qobject_cast<QAction*>(sender());
    if (!action) return;

    QString path = action->data().toString();
    if (path.isEmpty()) return;

    if (!maybeSaveProject()) return;

    QFileInfo fi(path);
    if (!fi.exists()) {
        QMessageBox::warning(this, tr("Project Not Found"),
            tr("The project file no longer exists:\n%1").arg(path));
        return;
    }

    if (!projectManager_.openProject(path)) {
        QMessageBox::critical(this, tr("Error"),
            tr("Failed to open project:\n%1").arg(path));
        return;
    }

    clips_.clear();
    clipModel_->refresh();
    
    loadProjectClips();
    updateWindowTitle();
    statusBar()->showMessage(tr("Opened project: %1").arg(fi.baseName()));
}

void MainWindow::openProjectSettings() {
    if (!projectManager_.hasProject()) {
        QMessageBox::information(this, tr("No Project"),
            tr("No project is currently open. Use File → New Project to create one."));
        return;
    }

    auto& project = projectManager_.project();
    ProjectSettingsDialog dialog(this);
    dialog.loadFromProject(project);

    if (dialog.exec() == QDialog::Accepted) {
        dialog.applyToProject(project);
        // Project is automatically marked dirty when modified
        updateWindowTitle();
        
        // Update output panel with game folder
        if (outputPanel_) {
            outputPanel_->setOutputFolder(QString::fromStdString(project.gameFolder()));
        }
        
        statusBar()->showMessage(tr("Project settings updated"));
    }
}

void MainWindow::updateWindowTitle() {
    QString title = tr("Woosh - Audio Batch Editor");
    
    if (projectManager_.hasProject()) {
        title = QString::fromStdString(projectManager_.project().name());
        if (projectManager_.isDirty()) {
            title += " *";
        }
        title += " - Woosh";
    }
    
    setWindowTitle(title);
}

void MainWindow::updateRecentProjectsMenu() {
    if (!recentProjectsMenu_) return;

    recentProjectsMenu_->clear();

    const auto recentProjects = projectManager_.recentProjects();
    
    if (recentProjects.isEmpty()) {
        auto* emptyAction = recentProjectsMenu_->addAction(tr("(No recent projects)"));
        emptyAction->setEnabled(false);
        return;
    }

    for (const QString& path : recentProjects) {
        QFileInfo fi(path);
        QString displayName = fi.baseName();
        auto* action = recentProjectsMenu_->addAction(displayName);
        action->setData(path);
        action->setToolTip(path);
        connect(action, &QAction::triggered, this, &MainWindow::openRecentProject);
    }
}

bool MainWindow::maybeSaveProject() {
    if (!projectManager_.isDirty()) return true;
    if (!projectManager_.hasProject()) return true;

    auto& project = projectManager_.project();

    QMessageBox::StandardButton result = QMessageBox::question(
        this, tr("Save Project?"),
        tr("The project '%1' has unsaved changes.\n\nDo you want to save before continuing?")
            .arg(QString::fromStdString(project.name())),
        QMessageBox::Save | QMessageBox::Discard | QMessageBox::Cancel,
        QMessageBox::Save
    );

    switch (result) {
        case QMessageBox::Save:
            saveProject();
            return !projectManager_.isDirty();  // Return true if save succeeded
        case QMessageBox::Discard:
            return true;
        case QMessageBox::Cancel:
        default:
            return false;
    }
}

void MainWindow::loadProjectClips() {
    if (!projectManager_.hasProject()) return;

    auto& project = projectManager_.project();
    QString rawFolder = QString::fromStdString(project.rawFolder());
    if (rawFolder.isEmpty()) return;

    // Scan RAW folder for audio files
    FileScanner scanner;
    auto found = scanner.scan(rawFolder.toStdString());

    if (found.empty()) {
        statusBar()->showMessage(tr("No audio files found in RAW folder"));
        return;
    }

    QStringList list;
    list.reserve(static_cast<int>(found.size()));
    for (const auto& p : found) {
        list << QString::fromStdString(p);
    }

    loadFileList(list);

    // Update output panel with game folder
    if (outputPanel_ && !project.gameFolder().empty()) {
        outputPanel_->setOutputFolder(QString::fromStdString(project.gameFolder()));
    }
}

void MainWindow::closeEvent(QCloseEvent* event) {
    if (!maybeSaveProject()) {
        event->ignore();
        return;
    }
    QMainWindow::closeEvent(event);
}

void MainWindow::onProjectChanged() {
    updateWindowTitle();
    updateRecentProjectsMenu();
}

void MainWindow::onDirtyStateChanged(bool dirty) {
    Q_UNUSED(dirty);
    updateWindowTitle();
}

void MainWindow::onRecentProjectsChanged() {
    updateRecentProjectsMenu();
}

void MainWindow::openSettings() {
    SettingsDialog dialog(this);
    dialog.setShowColumnTooltips(showColumnTooltips_);
    dialog.setDefaultAuthorName(defaultAuthorName_);

    connect(&dialog, &SettingsDialog::clearHistoryRequested, this, &MainWindow::onClearHistory);

    if (dialog.exec() == QDialog::Accepted) {
        showColumnTooltips_ = dialog.showColumnTooltips();
        defaultAuthorName_ = dialog.defaultAuthorName();
        clipModel_->setShowTooltips(showColumnTooltips_);
    }
}

void MainWindow::onClearHistory() {
    clearRecentHistory();
}

void MainWindow::keyPressEvent(QKeyEvent* event) {
    if (event->key() == Qt::Key_Space && !event->isAutoRepeat()) {
        onPlayPause();
        event->accept();
        return;
    }
    QMainWindow::keyPressEvent(event);
}

// ============================================================================
// File loading
// ============================================================================

void MainWindow::openFiles() {
    QStringList files = QFileDialog::getOpenFileNames(
        this, tr("Open Audio Files"), lastOpenDirectory_,
        tr("Audio Files (*.wav *.mp3 *.WAV *.MP3)")
    );

    if (files.isEmpty()) return;

    QFileInfo info(files.first());
    lastOpenDirectory_ = info.absolutePath();

    loadFileList(files);

    // Add to recent files
    for (const QString& f : files) {
        addRecentFile(f);
    }
}

void MainWindow::openFolder() {
    QString dir = QFileDialog::getExistingDirectory(
        this, tr("Open Folder"), lastOpenDirectory_
    );

    if (dir.isEmpty()) return;

    lastOpenDirectory_ = dir;

    FileScanner scanner;
    auto found = scanner.scan(dir.toStdString());

    if (found.empty()) {
        QMessageBox::information(this, tr("No Files Found"),
            tr("No audio files (WAV/MP3) found in the selected folder."));
        return;
    }

    QStringList list;
    list.reserve(static_cast<int>(found.size()));
    for (const auto& path : found) {
        list << QString::fromStdString(path);
    }

    loadFileList(list);
    addRecentFolder(dir);
}

void MainWindow::loadFileList(const QStringList& paths) {
    // Prevent starting a new load while one is in progress
    if (loadWatcher_ && loadWatcher_->isRunning()) {
        QMessageBox::warning(this, tr("Busy"), tr("A loading operation is already in progress."));
        return;
    }

    progressBar_->setMaximum(0);  // Indeterminate progress
    progressBar_->setVisible(true);
    statusBar()->showMessage(tr("Loading %1 file(s) in parallel...").arg(paths.size()));

    // Capture engine pointer for the lambda (engine_ lifetime is tied to MainWindow)
    AudioEngine* engine = &engine_;

    // Create watcher if needed
    if (!loadWatcher_) {
        loadWatcher_ = new QFutureWatcher<std::vector<AudioClip>>(this);
        connect(loadWatcher_, &QFutureWatcher<std::vector<AudioClip>>::finished,
                this, &MainWindow::onLoadingFinished);
    }

    // Convert QStringList to std::vector for QtConcurrent
    std::vector<QString> pathVec(paths.begin(), paths.end());

    // Load files in parallel using all available CPU cores
    QFuture<std::vector<AudioClip>> future = QtConcurrent::run([engine, pathVec = std::move(pathVec)]() {
        // Use QtConcurrent::mapped internally for true parallel loading
        QList<QString> pathList;
        pathList.reserve(static_cast<qsizetype>(pathVec.size()));
        for (const auto& p : pathVec) {
            pathList.append(p);
        }

        // Map: load each file in parallel
        QFuture<std::optional<AudioClip>> mappedFuture = QtConcurrent::mapped(pathList,
            [engine](const QString& path) -> std::optional<AudioClip> {
                auto clipOpt = engine->loadClip(path.toStdString());
                if (clipOpt) {
                    clipOpt->saveOriginal();
                }
                return clipOpt;
            });

        // Wait for all loads to complete
        mappedFuture.waitForFinished();

        // Collect results (filter out failed loads)
        std::vector<AudioClip> loadedClips;
        loadedClips.reserve(pathVec.size());
        for (const auto& clipOpt : mappedFuture.results()) {
            if (clipOpt) {
                loadedClips.push_back(std::move(*clipOpt));
            }
        }
        return loadedClips;
    });

    loadWatcher_->setFuture(future);
}

void MainWindow::onLoadingFinished() {
    progressBar_->setVisible(false);

    if (!loadWatcher_) return;

    std::vector<AudioClip> loadedClips = loadWatcher_->result();
    int loaded = static_cast<int>(loadedClips.size());

    for (auto& clip : loadedClips) {
        // Register clip with project if we have one
        if (projectManager_.hasProject()) {
            std::string relativePath = clip.displayName();
            // Only add if not already tracked
            if (!projectManager_.project().findClipState(relativePath)) {
                ClipState state;
                state.relativePath = relativePath;
                projectManager_.project().addClipState(state);
            }
        }
        clips_.push_back(std::move(clip));
    }

    clipModel_->refresh();

    QString msg = tr("Loaded %1 clip(s)").arg(loaded);
    statusBar()->showMessage(msg);
}

// ============================================================================
// Selection handling
// ============================================================================

void MainWindow::onSelectionChanged() {
    if (audioPlayer_->isPlaying()) {
        audioPlayer_->stop();
    }

    auto indices = selectedIndices();
    if (indices.empty()) {
        statusLabel_->clear();
        waveformView_->setClip(nullptr);
        audioPlayer_->setClip(nullptr);
        transportPanel_->setTimeDisplay(0, 0);
        undoAction_->setEnabled(false);
        return;
    }

    int idx = indices.front();
    if (idx >= 0 && idx < static_cast<int>(clips_.size())) {
        AudioClip* clip = &clips_[static_cast<size_t>(idx)];

        statusLabel_->setText(
            tr("%1 | %2s | %3 Hz | %4 ch | Peak: %5 dB | RMS: %6 dB")
                .arg(QString::fromStdString(clip->displayName()))
                .arg(clip->durationSeconds(), 0, 'f', 2)
                .arg(clip->sampleRate())
                .arg(clip->channels())
                .arg(clip->peakDb(), 0, 'f', 2)
                .arg(clip->rmsDb(), 0, 'f', 2)
        );

        waveformView_->setClip(clip);
        audioPlayer_->setClip(clip);
        updateTimeDisplay();
        undoAction_->setEnabled(clip->hasOriginal() && clip->isModified());
    }
}

std::vector<int> MainWindow::selectedIndices() const {
    std::vector<int> result;
    auto selected = clipTable_->selectionModel()->selectedRows();
    result.reserve(static_cast<size_t>(selected.size()));
    for (const auto& proxyIdx : selected) {
        QModelIndex srcIdx = proxyModel_->mapToSource(proxyIdx);
        result.push_back(srcIdx.row());
    }
    return result;
}

std::vector<int> MainWindow::allIndices() const {
    std::vector<int> result;
    result.reserve(clips_.size());
    for (size_t i = 0; i < clips_.size(); ++i) {
        result.push_back(static_cast<int>(i));
    }
    return result;
}

AudioClip* MainWindow::currentClip() {
    int idx = currentClipIndex();
    if (idx < 0) return nullptr;
    return &clips_[static_cast<size_t>(idx)];
}

int MainWindow::currentClipIndex() const {
    auto indices = selectedIndices();
    if (indices.empty()) return -1;
    return indices.front();
}

void MainWindow::updateWaveformView() {
    AudioClip* clip = currentClip();
    waveformView_->setClip(clip);
    if (clip) {
        updateTimeDisplay();
    }
}

void MainWindow::updateTimeDisplay() {
    AudioClip* clip = currentClip();
    if (!clip) {
        transportPanel_->setTimeDisplay(0, 0);
        return;
    }

    double totalSecs = clip->durationSeconds();
    double currentSecs = 0;

    if (clip->sampleRate() > 0) {
        currentSecs = static_cast<double>(audioPlayer_->positionFrame()) / clip->sampleRate();
    }

    transportPanel_->setTimeDisplay(currentSecs, totalSecs);
}

void MainWindow::refreshModelPreservingSelection() {
    // Save current selection
    auto indices = selectedIndices();

    // Refresh the model (this clears selection)
    clipModel_->refresh();

    // Restore selection
    selectRows(indices);
}

void MainWindow::selectRows(const std::vector<int>& indices) {
    if (indices.empty()) return;

    QItemSelection selection;
    for (int srcRow : indices) {
        QModelIndex srcIndex = clipModel_->index(srcRow, 0);
        QModelIndex proxyIndex = proxyModel_->mapFromSource(srcIndex);
        if (proxyIndex.isValid()) {
            // Select entire row
            QModelIndex firstCol = proxyModel_->index(proxyIndex.row(), 0);
            QModelIndex lastCol = proxyModel_->index(proxyIndex.row(), clipModel_->columnCount() - 1);
            selection.select(firstCol, lastCol);
        }
    }

    clipTable_->selectionModel()->select(selection,
        QItemSelectionModel::ClearAndSelect | QItemSelectionModel::Rows);

    // Scroll to first selected item
    if (!indices.empty()) {
        QModelIndex srcIndex = clipModel_->index(indices.front(), 0);
        QModelIndex proxyIndex = proxyModel_->mapFromSource(srcIndex);
        if (proxyIndex.isValid()) {
            clipTable_->scrollTo(proxyIndex);
        }
    }
}

// ============================================================================
// Playback
// ============================================================================

void MainWindow::onPlayPause() {
    if (!audioPlayer_->clip()) {
        AudioClip* clip = currentClip();
        if (clip) {
            audioPlayer_->setClip(clip);
        }
    }
    audioPlayer_->togglePlayPause();
}

void MainWindow::onStop() {
    audioPlayer_->stop();
    waveformView_->clearPlayhead();
    updateTimeDisplay();
}

void MainWindow::onSeek(int frame) {
    audioPlayer_->seek(frame);
    waveformView_->setPlayheadFrame(frame);
    updateTimeDisplay();
}

void MainWindow::onPlaybackPositionChanged(int frame) {
    waveformView_->setPlayheadFrame(frame);
    updateTimeDisplay();
}

void MainWindow::onPlaybackFinished() {
    waveformView_->clearPlayhead();
    updateTimeDisplay();
}

// ============================================================================
// Trim
// ============================================================================

void MainWindow::onTrimChanged(int startFrame, int endFrame) {
    bool hasTrim = (startFrame > 0 || endFrame > 0);
    transportPanel_->setTrimEnabled(hasTrim);

    // Update playback region to match trim markers
    audioPlayer_->setPlaybackRegion(startFrame, endFrame);
}

void MainWindow::onApplyTrim() {
    AudioClip* clip = currentClip();
    if (!clip) return;

    int startFrame = waveformView_->trimStartFrame();
    int endFrame = waveformView_->trimEndFrame();

    if (startFrame <= 0 && endFrame <= 0) return;

    int channels = clip->channels();
    int maxFrame = static_cast<int>(clip->frameCount());
    int effectiveEnd = (endFrame > 0) ? endFrame : maxFrame;

    if (startFrame >= effectiveEnd) return;

    // Calculate trim times in seconds before applying
    double sampleRate = static_cast<double>(clip->sampleRate());
    double trimStartSec = static_cast<double>(startFrame) / sampleRate;
    double trimEndSec = static_cast<double>(effectiveEnd) / sampleRate;

    auto& samples = clip->samplesMutable();
    size_t startSample = static_cast<size_t>(startFrame * channels);
    size_t endSample = static_cast<size_t>(effectiveEnd * channels);

    if (startSample >= samples.size()) return;
    endSample = std::min(endSample, samples.size());

    std::vector<float> trimmed(samples.begin() + startSample, samples.begin() + endSample);
    clip->setSamples(std::move(trimmed));
    clip->setModified(true);

    engine_.updateClipMetrics(*clip);

    // Update project clip state if we have a project
    if (projectManager_.hasProject()) {
        std::string relativePath = clip->displayName();
        projectManager_.project().updateClipState(relativePath, 
            [trimStartSec, trimEndSec](ClipState& state) {
                state.isTrimmed = true;
                state.trimStartSec = trimStartSec;
                state.trimEndSec = trimEndSec;
            });
        projectManager_.project().markDirty();
    }

    waveformView_->clearTrim();
    waveformView_->setClip(clip);
    audioPlayer_->setClip(clip);
    audioPlayer_->setPlaybackRegion(0, 0);
    refreshModelPreservingSelection();

    undoAction_->setEnabled(clip->hasOriginal());
    statusBar()->showMessage(tr("Applied trim to %1").arg(QString::fromStdString(clip->displayName())));
}

// ============================================================================
// Undo processing
// ============================================================================

void MainWindow::onUndoProcessing() {
    AudioClip* clip = currentClip();
    if (!clip || !clip->hasOriginal()) return;

    clip->restoreOriginal();

    waveformView_->setClip(clip);
    audioPlayer_->setClip(clip);
    refreshModelPreservingSelection();

    undoAction_->setEnabled(false);
    statusBar()->showMessage(tr("Restored original for %1").arg(QString::fromStdString(clip->displayName())));
}

// ============================================================================
// Processing
// ============================================================================

void MainWindow::onNormalizeSelected() {
    auto indices = selectedIndices();
    if (indices.empty()) {
        QMessageBox::information(this, tr("No Selection"),
            tr("Please select one or more clips to normalize."));
        return;
    }
    applyProcessing(indices, true, false);
}

void MainWindow::onNormalizeAll() {
    if (clips_.empty()) {
        QMessageBox::information(this, tr("No Clips"),
            tr("Please load some audio files first."));
        return;
    }
    applyProcessing(allIndices(), true, false);
}

void MainWindow::onCompressSelected() {
    auto indices = selectedIndices();
    if (indices.empty()) {
        QMessageBox::information(this, tr("No Selection"),
            tr("Please select one or more clips to compress."));
        return;
    }
    applyProcessing(indices, false, true);
}

void MainWindow::onCompressAll() {
    if (clips_.empty()) {
        QMessageBox::information(this, tr("No Clips"),
            tr("Please load some audio files first."));
        return;
    }
    applyProcessing(allIndices(), false, true);
}

void MainWindow::applyProcessing(const std::vector<int>& indices, bool normalize, bool compress) {
    // Prevent starting new processing while one is in progress
    if (processWatcher_ && processWatcher_->isRunning()) {
        QMessageBox::warning(this, tr("Busy"), tr("A processing operation is already in progress."));
        return;
    }

    progressBar_->setMaximum(0);  // Indeterminate progress
    progressBar_->setVisible(true);
    statusBar()->showMessage(tr("Processing %1 clip(s) in parallel...").arg(indices.size()));

    // Store indices for use in completion handler
    processingIndices_ = indices;

    float normTarget = static_cast<float>(processingPanel_->normalizeTarget());
    float threshold = processingPanel_->compThreshold();
    float ratio = processingPanel_->compRatio();
    float attack = processingPanel_->compAttackMs();
    float release = processingPanel_->compReleaseMs();
    float makeup = processingPanel_->compMakeupDb();

    // Store processing parameters for async completion handler
    processingAppliedNormalize_ = normalize;
    processingAppliedCompress_ = compress;
    processingNormalizeTarget_ = normTarget;
    processingCompThreshold_ = threshold;
    processingCompRatio_ = ratio;
    processingCompAttackMs_ = attack;
    processingCompReleaseMs_ = release;
    processingCompMakeupDb_ = makeup;

    // Collect clips to process (make copies for thread safety)
    std::vector<AudioClip> clipsToProcess;
    clipsToProcess.reserve(indices.size());
    for (int idx : indices) {
        if (idx >= 0 && idx < static_cast<int>(clips_.size())) {
            clipsToProcess.push_back(clips_[static_cast<size_t>(idx)]);
        }
    }

    // Create watcher if needed
    if (!processWatcher_) {
        processWatcher_ = new QFutureWatcher<std::vector<AudioClip>>(this);
        connect(processWatcher_, &QFutureWatcher<std::vector<AudioClip>>::finished,
                this, &MainWindow::onProcessingFinished);
    }

    // Capture engine pointer
    AudioEngine* engine = &engine_;

    // Process clips in parallel
    QFuture<std::vector<AudioClip>> future = QtConcurrent::run(
        [engine, clipsToProcess = std::move(clipsToProcess), normalize, compress,
         normTarget, threshold, ratio, attack, release, makeup]() mutable {
            
            // Convert to QList for QtConcurrent::mapped
            QList<AudioClip> clipList;
            clipList.reserve(static_cast<qsizetype>(clipsToProcess.size()));
            for (auto& clip : clipsToProcess) {
                clipList.append(std::move(clip));
            }

            // Map: process each clip in parallel
            QFuture<AudioClip> mappedFuture = QtConcurrent::mapped(clipList,
                [engine, normalize, compress, normTarget, threshold, ratio, attack, release, makeup]
                (AudioClip clip) -> AudioClip {
                    if (normalize) {
                        engine->normalizeToPeak(clip, normTarget);
                    }
                    if (compress) {
                        engine->compress(clip, threshold, ratio, attack, release, makeup);
                    }
                    clip.setModified(true);
                    return clip;
                });

            // Wait for all processing to complete
            mappedFuture.waitForFinished();

            // Collect results
            std::vector<AudioClip> results;
            results.reserve(clipList.size());
            for (const auto& clip : mappedFuture.results()) {
                results.push_back(clip);
            }
            return results;
        });

    processWatcher_->setFuture(future);
}

void MainWindow::onProcessingFinished() {
    progressBar_->setVisible(false);

    if (!processWatcher_) return;

    std::vector<AudioClip> processedClips = processWatcher_->result();
    
    // Update clips with processed data
    for (size_t i = 0; i < processedClips.size() && i < processingIndices_.size(); ++i) {
        int idx = processingIndices_[i];
        if (idx >= 0 && idx < static_cast<int>(clips_.size())) {
            clips_[static_cast<size_t>(idx)] = std::move(processedClips[i]);
            
            // Update project clip state if we have a project
            if (projectManager_.hasProject()) {
                std::string relativePath = clips_[static_cast<size_t>(idx)].displayName();
                bool appliedNormalize = processingAppliedNormalize_;
                bool appliedCompress = processingAppliedCompress_;
                float normTarget = processingNormalizeTarget_;
                float threshold = processingCompThreshold_;
                float ratio = processingCompRatio_;
                float attack = processingCompAttackMs_;
                float release = processingCompReleaseMs_;
                float makeup = processingCompMakeupDb_;
                
                projectManager_.project().updateClipState(relativePath, 
                    [appliedNormalize, appliedCompress, normTarget, threshold, 
                     ratio, attack, release, makeup](ClipState& state) {
                        if (appliedNormalize) {
                            state.isNormalized = true;
                            state.normalizeTargetDb = static_cast<double>(normTarget);
                        }
                        if (appliedCompress) {
                            state.isCompressed = true;
                            state.compressorSettings.threshold = threshold;
                            state.compressorSettings.ratio = ratio;
                            state.compressorSettings.attackMs = attack;
                            state.compressorSettings.releaseMs = release;
                            state.compressorSettings.makeupDb = makeup;
                        }
                    });
            }
        }
    }

    // Mark project dirty if we updated clip states
    if (projectManager_.hasProject() && !processingIndices_.empty()) {
        projectManager_.project().markDirty();
    }

    refreshModelPreservingSelection();

    int currentIdx = currentClipIndex();
    if (currentIdx >= 0 && std::find(processingIndices_.begin(), processingIndices_.end(), currentIdx) != processingIndices_.end()) {
        updateWaveformView();
        undoAction_->setEnabled(currentClip() && currentClip()->hasOriginal());
    }

    statusBar()->showMessage(tr("Processed %1 clip(s)").arg(processedClips.size()));
    processingIndices_.clear();
}

// ============================================================================
// Export
// ============================================================================

void MainWindow::onExportSelected() {
    auto indices = selectedIndices();
    if (indices.empty()) {
        QMessageBox::information(this, tr("No Selection"),
            tr("Please select one or more clips to export."));
        return;
    }
    exportClips(indices);
}

void MainWindow::onExportAll() {
    if (clips_.empty()) {
        QMessageBox::information(this, tr("No Clips"),
            tr("Please load some audio files first."));
        return;
    }
    exportClips(allIndices());
}

void MainWindow::exportClips(const std::vector<int>& indices) {
    QString outputDir = outputPanel_->outputFolder();
    bool overwrite = outputPanel_->overwriteOriginals();

    if (outputDir.isEmpty() && !overwrite) {
        QMessageBox::warning(this, tr("No Output Folder"),
            tr("Please select an output folder or enable 'Overwrite original files'."));
        return;
    }

    // Prevent starting a new export while one is in progress
    if (exportWatcher_ && exportWatcher_->isRunning()) {
        QMessageBox::warning(this, tr("Busy"), tr("An export operation is already in progress."));
        return;
    }

    progressBar_->setMaximum(0);  // Indeterminate progress
    progressBar_->setVisible(true);
    statusBar()->showMessage(tr("Exporting %1 file(s)...").arg(indices.size()));

    // Get export settings from project (or use defaults)
    ExportFormat exportFormat = ExportFormat::WAV;
    Mp3Encoder::BitrateMode bitrate = Mp3Encoder::BitrateMode::CBR_160;
    Mp3Metadata metadata;
    metadata.comment = "Made by Woosh";

    if (projectManager_.hasProject()) {
        const auto& exportSettings = projectManager_.project().exportSettings();
        exportFormat = exportSettings.format;
        
        switch (exportSettings.mp3Bitrate) {
            case 128: bitrate = Mp3Encoder::BitrateMode::CBR_128; break;
            case 160: bitrate = Mp3Encoder::BitrateMode::CBR_160; break;
            case 192: bitrate = Mp3Encoder::BitrateMode::CBR_192; break;
            default:  bitrate = Mp3Encoder::BitrateMode::CBR_160; break;
        }
        
        metadata.artist = exportSettings.authorName;
        metadata.album = exportSettings.gameName;
        metadata.comment = "Made by Woosh";
    }

    // Collect clips and destination info for background thread
    struct ExportItem {
        AudioClip clip;  // Copy of clip data
        std::string destFolder;
    };
    std::vector<ExportItem> items;
    items.reserve(indices.size());

    for (int idx : indices) {
        if (idx < 0 || idx >= static_cast<int>(clips_.size())) continue;

        const auto& clip = clips_[static_cast<size_t>(idx)];
        std::string destFolder;

        if (overwrite) {
            QFileInfo fi(QString::fromStdString(clip.filePath()));
            destFolder = fi.absolutePath().toStdString();
        } else {
            destFolder = outputDir.toStdString();
        }

        items.push_back({clip, destFolder});
    }

    // Create watcher if needed
    if (!exportWatcher_) {
        exportWatcher_ = new QFutureWatcher<int>(this);
        connect(exportWatcher_, &QFutureWatcher<int>::finished,
                this, &MainWindow::onExportFinished);
    }

    // Capture engine pointer for the lambda
    AudioEngine* engine = &engine_;

    // Run exports in parallel using all available CPU cores
    QFuture<int> future = QtConcurrent::run(
        [engine, items = std::move(items), exportFormat, bitrate, metadata]() {
        // Convert to QList for QtConcurrent::mapped
        QList<ExportItem> itemList;
        itemList.reserve(static_cast<qsizetype>(items.size()));
        for (auto& item : items) {
            itemList.append(std::move(item));
        }

        // Map: export each file in parallel
        QFuture<bool> mappedFuture = QtConcurrent::mapped(itemList,
            [engine, exportFormat, bitrate, metadata](const ExportItem& item) -> bool {
                switch (exportFormat) {
                    case ExportFormat::MP3:
                        return engine->exportMp3(item.clip, item.destFolder, bitrate, metadata);
                    case ExportFormat::OGG:
                        // OGG export not yet implemented, fall back to WAV
                        return engine->exportWav(item.clip, item.destFolder);
                    case ExportFormat::WAV:
                    default:
                        return engine->exportWav(item.clip, item.destFolder);
                }
            });

        // Wait for all exports to complete
        mappedFuture.waitForFinished();

        // Count successes
        int exported = 0;
        for (bool success : mappedFuture.results()) {
            if (success) ++exported;
        }
        return exported;
    });

    exportWatcher_->setFuture(future);
}

void MainWindow::onExportFinished() {
    progressBar_->setVisible(false);

    if (!exportWatcher_) return;

    int exported = exportWatcher_->result();
    statusBar()->showMessage(tr("Exported %1 clip(s)").arg(exported));
}
