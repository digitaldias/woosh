#include "MainWindow.h"

#include <QAction>
#include <QFileDialog>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QFormLayout>
#include <QMenuBar>
#include <QMessageBox>
#include <QProgressBar>
#include <QSplitter>
#include <QStatusBar>
#include <QVBoxLayout>
#include <QWidget>
#include <QFormLayout>
#include <QHBoxLayout>
#include "ui/BatchDialog.h"
#include "ui/WaveformView.h"

namespace {
QString formatStats(const AudioClip& clip) {
    return QString("%1 | %2 Hz | %3 ch | %4 s | peak %5 dBFS | rms %6 dB")
        .arg(clip.displayName())
        .arg(clip.sampleRate())
        .arg(clip.channels())
        .arg(QString::number(clip.durationSeconds(), 'f', 2))
        .arg(QString::number(clip.peakDb(), 'f', 2))
        .arg(QString::number(clip.rmsDb(), 'f', 2));
}
}

MainWindow::MainWindow(QWidget* parent) : QMainWindow(parent) {
    setupUi();
    setupMenus();
    statusBar()->showMessage("Ready");
}

void MainWindow::setupUi() {
    auto* central = new QWidget(this);
    auto* layout = new QVBoxLayout();
    auto* splitter = new QSplitter(Qt::Horizontal, central);

    fileList_ = new QListWidget(splitter);
    fileList_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    connect(fileList_, &QListWidget::itemSelectionChanged, this, &MainWindow::updateSelection);

    auto* rightPane = new QWidget(splitter);
    auto* rightLayout = new QVBoxLayout(rightPane);

    waveformView_ = new WaveformView(rightPane);
    waveformView_->setMinimumHeight(200);
    rightLayout->addWidget(waveformView_);

    auto* form = new QFormLayout();
    startTrim_ = new QLineEdit("0.0", rightPane);
    endTrim_ = new QLineEdit("0.0", rightPane);
    normalizeTarget_ = new QLineEdit("-1.0", rightPane);
    compThreshold_ = new QLineEdit("-6.0", rightPane);
    compRatio_ = new QLineEdit("2.0", rightPane);
    compAttack_ = new QLineEdit("10.0", rightPane);
    compRelease_ = new QLineEdit("80.0", rightPane);
    compMakeup_ = new QLineEdit("0.0", rightPane);

    form->addRow("Trim start (s)", startTrim_);
    form->addRow("Trim end (s)", endTrim_);
    form->addRow("Normalize target dBFS", normalizeTarget_);
    form->addRow("Comp threshold dB", compThreshold_);
    form->addRow("Comp ratio", compRatio_);
    form->addRow("Comp attack ms", compAttack_);
    form->addRow("Comp release ms", compRelease_);
    form->addRow("Comp makeup dB", compMakeup_);
    rightLayout->addLayout(form);

    splitter->addWidget(fileList_);
    splitter->addWidget(rightPane);
    splitter->setStretchFactor(1, 1);

    progressBar_ = new QProgressBar(central);
    progressBar_->setMinimum(0);
    progressBar_->setMaximum(100);
    progressBar_->setValue(0);

    layout->addWidget(splitter);
    layout->addWidget(progressBar_);
    central->setLayout(layout);
    setCentralWidget(central);

    statusLabel_ = new QLabel(this);
    statusBar()->addPermanentWidget(statusLabel_);
}

void MainWindow::setupMenus() {
    auto* fileMenu = menuBar()->addMenu("File");
    openFilesAction_ = fileMenu->addAction("Open Files...", this, &MainWindow::openFiles);
    openFolderAction_ = fileMenu->addAction("Open Folder...", this, &MainWindow::openFolder);
    fileMenu->addSeparator();
    exportAction_ = fileMenu->addAction("Export...", this, &MainWindow::exportSelection);

    auto* editMenu = menuBar()->addMenu("Edit");
    normalizeAction_ = editMenu->addAction("Normalize", this, &MainWindow::applyNormalize);
    trimAction_ = editMenu->addAction("Trim", this, &MainWindow::applyTrim);
    compressAction_ = editMenu->addAction("Compress", this, &MainWindow::applyCompress);

    auto* toolsMenu = menuBar()->addMenu("Tools");
    batchAction_ = toolsMenu->addAction("Batch Process", this, &MainWindow::openBatchDialog);

    auto* helpMenu = menuBar()->addMenu("Help");
    helpMenu->addAction("About", this, [this]() {
        QMessageBox::about(this, "Woosh", "Woosh - Game audio batch editor.\nMP3 export TODO.");
    });
}

void MainWindow::openFiles() {
    auto files = QFileDialog::getOpenFileNames(this, "Open Audio Files", QString(),
        "Audio Files (*.wav *.mp3)");
    if (!files.isEmpty()) {
        loadFileList(files);
    }
}

void MainWindow::openFolder() {
    auto dir = QFileDialog::getExistingDirectory(this, "Open Folder");
    if (dir.isEmpty()) return;
    FileScanner scanner;
    auto found = scanner.scan(dir.toStdString());
    QStringList list;
    for (const auto& path : found) list << QString::fromStdString(path);
    loadFileList(list);
}

void MainWindow::loadFileList(const QStringList& paths) {
    for (const auto& path : paths) {
        auto clipOpt = engine_.loadClip(path.toStdString());
        if (!clipOpt) {
            QMessageBox::warning(this, "Load failed", "Could not load " + path);
            continue;
        }
        clips_.push_back(std::move(*clipOpt));
    }
    refreshList();
}

void MainWindow::refreshList() {
    fileList_->clear();
    for (const auto& clip : clips_) {
        fileList_->addItem(formatStats(clip));
    }
    statusBar()->showMessage(QString("Loaded %1 clips").arg(clips_.size()));
}

std::optional<size_t> MainWindow::currentIndex() const {
    auto selected = fileList_->selectionModel()->selectedIndexes();
    if (selected.isEmpty()) return std::nullopt;
    return static_cast<size_t>(selected.first().row());
}

void MainWindow::exportSelection() {
    if (clips_.empty()) return;
    auto folder = QFileDialog::getExistingDirectory(this, "Select export folder");
    if (folder.isEmpty()) return;
    auto indices = fileList_->selectionModel()->selectedIndexes();
    if (indices.isEmpty()) {
        for (size_t i = 0; i < clips_.size(); ++i) {
            engine_.exportWav(clips_[i], folder.toStdString());
        }
    } else {
        for (const auto& idx : indices) {
            engine_.exportWav(clips_[static_cast<size_t>(idx.row())], folder.toStdString());
        }
    }
    statusBar()->showMessage("Export complete");
}

void MainWindow::applyNormalize() {
    auto idxOpt = currentIndex();
    if (!idxOpt) return;
    bool ok = false;
    const auto target = normalizeTarget_->text().toDouble(&ok);
    if (!ok) return;
    for (const auto& idx : fileList_->selectionModel()->selectedIndexes()) {
        auto& clip = clips_[static_cast<size_t>(idx.row())];
        engine_.normalizeToPeak(clip, static_cast<float>(target));
    }
    refreshList();
}

void MainWindow::applyTrim() {
    bool okStart = false, okEnd = false;
    double start = startTrim_->text().toDouble(&okStart);
    double end = endTrim_->text().toDouble(&okEnd);
    if (!okStart || !okEnd) return;
    for (const auto& idx : fileList_->selectionModel()->selectedIndexes()) {
        auto& clip = clips_[static_cast<size_t>(idx.row())];
        engine_.trim(clip, static_cast<float>(start), static_cast<float>(end));
    }
    refreshList();
}

void MainWindow::applyCompress() {
    bool okT=false, okR=false, okA=false, okRel=false, okM=false;
    float threshold = compThreshold_->text().toFloat(&okT);
    float ratio = compRatio_->text().toFloat(&okR);
    float attack = compAttack_->text().toFloat(&okA);
    float release = compRelease_->text().toFloat(&okRel);
    float makeup = compMakeup_->text().toFloat(&okM);
    if (!(okT && okR && okA && okRel && okM)) return;
    for (const auto& idx : fileList_->selectionModel()->selectedIndexes()) {
        auto& clip = clips_[static_cast<size_t>(idx.row())];
        engine_.compress(clip, threshold, ratio, attack, release, makeup);
    }
    refreshList();
}

void MainWindow::openBatchDialog() {
    BatchDialog dialog(engine_, clips_, this);
    dialog.exec();
    refreshList();
}

void MainWindow::updateSelection() {
    auto idxOpt = currentIndex();
    if (!idxOpt) return;
    const auto& clip = clips_[*idxOpt];
    statusLabel_->setText(formatStats(clip));
    waveformView_->setPlaceholderText(QString("Waveform for %1").arg(clip.displayName()));
}



