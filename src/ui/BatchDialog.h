#pragma once

#include <QDialog>
#include <vector>
#include "audio/AudioClip.h"
#include "audio/AudioEngine.h"

class QListWidget;
class QProgressBar;
class QPushButton;

class BatchDialog : public QDialog {
    Q_OBJECT
public:
    BatchDialog(AudioEngine& engine, std::vector<AudioClip>& clips, QWidget* parent = nullptr);
    ~BatchDialog() override = default;

private slots:
    void runBatch();

private:
    AudioEngine& engine_;
    std::vector<AudioClip>& clips_;
    QListWidget* list_ = nullptr;
    QProgressBar* progress_ = nullptr;
    QPushButton* runButton_ = nullptr;
};


