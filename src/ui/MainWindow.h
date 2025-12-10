#pragma once

#include <QMainWindow>
#include <memory>
#include <vector>
#include <optional>
#include "audio/AudioClip.h"
#include "audio/AudioEngine.h"
#include "utils/FileScanner.h"

class QListWidget;
class QLabel;
class QLineEdit;
class QProgressBar;
class QAction;
class WaveformView;
class BatchDialog;

class MainWindow : public QMainWindow {
    Q_OBJECT
public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override = default;

private slots:
    void openFiles();
    void openFolder();
    void exportSelection();
    void applyNormalize();
    void applyTrim();
    void applyCompress();
    void openBatchDialog();
    void updateSelection();

private:
    void setupUi();
    void setupMenus();
    void loadFileList(const QStringList& paths);
    void refreshList();
    std::optional<size_t> currentIndex() const;

    AudioEngine engine_;
    std::vector<AudioClip> clips_;

    QListWidget* fileList_ = nullptr;
    WaveformView* waveformView_ = nullptr;
    QLabel* statusLabel_ = nullptr;
    QLineEdit* startTrim_ = nullptr;
    QLineEdit* endTrim_ = nullptr;
    QLineEdit* normalizeTarget_ = nullptr;
    QLineEdit* compThreshold_ = nullptr;
    QLineEdit* compRatio_ = nullptr;
    QLineEdit* compAttack_ = nullptr;
    QLineEdit* compRelease_ = nullptr;
    QLineEdit* compMakeup_ = nullptr;
    QProgressBar* progressBar_ = nullptr;

    QAction* openFilesAction_ = nullptr;
    QAction* openFolderAction_ = nullptr;
    QAction* exportAction_ = nullptr;
    QAction* normalizeAction_ = nullptr;
    QAction* trimAction_ = nullptr;
    QAction* compressAction_ = nullptr;
    QAction* batchAction_ = nullptr;
};


