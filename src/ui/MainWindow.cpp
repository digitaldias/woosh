/**
 * @file MainWindow.cpp
 * @brief Implementation of the main application window.
 */

#include "MainWindow.h"

#include <QAction>
#include <QDialog>
#include <QDir>
#include <QFileDialog>
#include <QFileInfo>
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

#include "audio/AudioPlayer.h"
#include "ui/ClipTableModel.h"
#include "ui/OutputPanel.h"
#include "ui/ProcessingPanel.h"
#include "ui/SettingsDialog.h"
#include "ui/TransportPanel.h"
#include "ui/WaveformView.h"
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
    statusBar()->showMessage(tr("Ready"));
}

MainWindow::~MainWindow() {
    saveSettings();
}

void MainWindow::loadSettings() {
    lastOpenDirectory_ = settings_->value(kKeyLastDir, QDir::homePath()).toString();
    recentFiles_ = settings_->value(kKeyRecentFiles).toStringList();
    recentFolders_ = settings_->value(kKeyRecentFolders).toStringList();
}

void MainWindow::saveSettings() {
    settings_->setValue(kKeyLastDir, lastOpenDirectory_);
    settings_->setValue(kKeyRecentFiles, recentFiles_);
    settings_->setValue(kKeyRecentFolders, recentFolders_);
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
    clipModel_ = new ClipTableModel(clips_, this);
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

    processingPanel_ = new ProcessingPanel(rightPane);
    rightLayout->addWidget(processingPanel_);

    connect(processingPanel_, &ProcessingPanel::applyToSelected, this, &MainWindow::onApplyToSelected);
    connect(processingPanel_, &ProcessingPanel::applyToAll, this, &MainWindow::onApplyToAll);

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

    QByteArray geom = settings_->value(kKeyWindowGeom).toByteArray();
    if (!geom.isEmpty()) {
        restoreGeometry(geom);
    }
}

void MainWindow::setupMenus() {
    // --- File menu ---
    auto* fileMenu = menuBar()->addMenu(tr("&File"));

    openFilesAction_ = fileMenu->addAction(tr("&Open Files..."), this, &MainWindow::openFiles);
    openFilesAction_->setShortcut(QKeySequence::Open);

    openFolderAction_ = fileMenu->addAction(tr("Open &Folder..."), this, &MainWindow::openFolder);
    openFolderAction_->setShortcut(QKeySequence(Qt::CTRL | Qt::SHIFT | Qt::Key_O));

    fileMenu->addSeparator();

    // Recent Files submenu
    recentFilesMenu_ = fileMenu->addMenu(tr("Recent &Files"));
    updateRecentFilesMenu();

    // Recent Folders submenu
    recentFoldersMenu_ = fileMenu->addMenu(tr("Recent F&olders"));
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

void MainWindow::openSettings() {
    SettingsDialog dialog(this);
    dialog.setShowColumnTooltips(clipModel_->showTooltips());

    connect(&dialog, &SettingsDialog::clearHistoryRequested, this, &MainWindow::onClearHistory);

    if (dialog.exec() == QDialog::Accepted) {
        clipModel_->setShowTooltips(dialog.showColumnTooltips());
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
    int loaded = 0;
    int failed = 0;

    progressBar_->setMaximum(paths.size());
    progressBar_->setValue(0);
    progressBar_->setVisible(true);

    for (const auto& path : paths) {
        auto clipOpt = engine_.loadClip(path.toStdString());
        if (clipOpt) {
            clipOpt->saveOriginal();
            clips_.push_back(std::move(*clipOpt));
            ++loaded;
        } else {
            ++failed;
        }
        progressBar_->setValue(progressBar_->value() + 1);
    }

    progressBar_->setVisible(false);
    clipModel_->refresh();

    QString msg = tr("Loaded %1 clip(s)").arg(loaded);
    if (failed > 0) {
        msg += tr(", %1 failed").arg(failed);
    }
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

    auto& samples = clip->samplesMutable();
    size_t startSample = static_cast<size_t>(startFrame * channels);
    size_t endSample = static_cast<size_t>(effectiveEnd * channels);

    if (startSample >= samples.size()) return;
    endSample = std::min(endSample, samples.size());

    std::vector<float> trimmed(samples.begin() + startSample, samples.begin() + endSample);
    clip->setSamples(std::move(trimmed));
    clip->setModified(true);

    engine_.updateClipMetrics(*clip);

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

void MainWindow::onApplyToSelected(bool normalize, bool compress) {
    auto indices = selectedIndices();
    if (indices.empty()) {
        QMessageBox::information(this, tr("No Selection"),
            tr("Please select one or more clips to process."));
        return;
    }
    applyProcessing(indices, normalize, compress);
}

void MainWindow::onApplyToAll(bool normalize, bool compress) {
    if (clips_.empty()) {
        QMessageBox::information(this, tr("No Clips"),
            tr("Please load some audio files first."));
        return;
    }
    applyProcessing(allIndices(), normalize, compress);
}

void MainWindow::applyProcessing(const std::vector<int>& indices, bool normalize, bool compress) {
    progressBar_->setMaximum(static_cast<int>(indices.size()));
    progressBar_->setValue(0);
    progressBar_->setVisible(true);

    float normTarget = static_cast<float>(processingPanel_->normalizeTarget());
    float threshold = processingPanel_->compThreshold();
    float ratio = processingPanel_->compRatio();
    float attack = processingPanel_->compAttackMs();
    float release = processingPanel_->compReleaseMs();
    float makeup = processingPanel_->compMakeupDb();

    int processed = 0;
    for (int idx : indices) {
        if (idx < 0 || idx >= static_cast<int>(clips_.size())) continue;

        auto& clip = clips_[static_cast<size_t>(idx)];

        if (normalize) {
            engine_.normalizeToPeak(clip, normTarget);
        }
        if (compress) {
            engine_.compress(clip, threshold, ratio, attack, release, makeup);
        }

        clip.setModified(true);
        ++processed;
        progressBar_->setValue(processed);
    }

    progressBar_->setVisible(false);
    refreshModelPreservingSelection();

    int currentIdx = currentClipIndex();
    if (currentIdx >= 0 && std::find(indices.begin(), indices.end(), currentIdx) != indices.end()) {
        updateWaveformView();
        undoAction_->setEnabled(currentClip() && currentClip()->hasOriginal());
    }

    statusBar()->showMessage(tr("Processed %1 clip(s)").arg(processed));
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

    std::string destFolder = overwrite ? "" : outputDir.toStdString();

    progressBar_->setMaximum(static_cast<int>(indices.size()));
    progressBar_->setValue(0);
    progressBar_->setVisible(true);

    int exported = 0;
    for (int idx : indices) {
        if (idx < 0 || idx >= static_cast<int>(clips_.size())) continue;

        auto& clip = clips_[static_cast<size_t>(idx)];

        if (overwrite) {
            QFileInfo fi(QString::fromStdString(clip.filePath()));
            destFolder = fi.absolutePath().toStdString();
        }

        engine_.exportWav(clip, destFolder);
        ++exported;
        progressBar_->setValue(exported);
    }

    progressBar_->setVisible(false);
    statusBar()->showMessage(tr("Exported %1 clip(s)").arg(exported));
}
