#include "BatchDialog.h"

#include <QVBoxLayout>
#include <QListWidget>
#include <QProgressBar>
#include <QPushButton>
#include <QFileDialog>
#include <QLabel>

BatchDialog::BatchDialog(AudioEngine& engine, std::vector<AudioClip>& clips, QWidget* parent)
    : QDialog(parent), engine_(engine), clips_(clips) {
    setWindowTitle("Batch Process");
    auto* layout = new QVBoxLayout(this);
    layout->addWidget(new QLabel("Files to process:", this));

    list_ = new QListWidget(this);
    for (const auto& clip : clips_) list_->addItem(QString::fromStdString(clip.filePath()));
    list_->setSelectionMode(QAbstractItemView::ExtendedSelection);
    layout->addWidget(list_);

    progress_ = new QProgressBar(this);
    progress_->setRange(0, 100);
    layout->addWidget(progress_);

    runButton_ = new QPushButton("Run", this);
    layout->addWidget(runButton_);
    connect(runButton_, &QPushButton::clicked, this, &BatchDialog::runBatch);
}

void BatchDialog::runBatch() {
    auto folder = QFileDialog::getExistingDirectory(this, "Select export folder");
    if (folder.isEmpty()) return;
    auto selected = list_->selectedIndexes();
    std::vector<size_t> indices;
    if (selected.isEmpty()) {
        for (size_t i = 0; i < clips_.size(); ++i) indices.push_back(i);
    } else {
        for (const auto& idx : selected) indices.push_back(static_cast<size_t>(idx.row()));
    }

    int total = static_cast<int>(indices.size());
    int processed = 0;
    for (auto index : indices) {
        engine_.exportWav(clips_[index], folder.toStdString());
        ++processed;
        progress_->setValue(static_cast<int>((processed * 100.0f) / total));
        qApp->processEvents(); // TODO: replace with worker thread/QtConcurrent
    }
    accept();
}


