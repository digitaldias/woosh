/**
 * @file MainWindow.h
 * @brief Main application window for Woosh audio batch editor.
 *
 * Provides the primary UI: a sortable clip table on the left, waveform preview
 * with playback/trim controls, and processing/output panels on the right.
 */

#pragma once

#include <QMainWindow>
#include <QString>
#include <QStringList>
#include <memory>
#include <vector>

#include "audio/AudioClip.h"
#include "audio/AudioEngine.h"

class QTableView;
class QSortFilterProxyModel;
class QLabel;
class QProgressBar;
class QAction;
class QMenu;
class QSettings;

class AudioPlayer;
class ClipTableModel;
class ProcessingPanel;
class OutputPanel;
class TransportPanel;
class WaveformView;

/**
 * @class MainWindow
 * @brief The main application window for Woosh.
 */
class MainWindow : public QMainWindow {
    Q_OBJECT

public:
    explicit MainWindow(QWidget* parent = nullptr);
    ~MainWindow() override;

protected:
    void keyPressEvent(QKeyEvent* event) override;

private Q_SLOTS:
    // File menu actions
    void openFiles();
    void openFolder();
    void openRecentFile();
    void openRecentFolder();
    void openSettings();

    // Processing actions
    void onApplyToSelected(bool normalize, bool compress);
    void onApplyToAll(bool normalize, bool compress);

    // Export actions
    void onExportSelected();
    void onExportAll();

    // Selection changed in table
    void onSelectionChanged();

    // Transport/playback
    void onPlayPause();
    void onStop();
    void onSeek(int frame);
    void onPlaybackPositionChanged(int frame);
    void onPlaybackFinished();

    // Waveform/trim
    void onTrimChanged(int startFrame, int endFrame);
    void onApplyTrim();

    // Undo processing
    void onUndoProcessing();

    // Settings
    void onClearHistory();

private:
    void setupUi();
    void setupMenus();
    void loadSettings();
    void saveSettings();

    // Recent files/folders management
    void addRecentFile(const QString& path);
    void addRecentFolder(const QString& path);
    void updateRecentFilesMenu();
    void updateRecentFoldersMenu();
    void clearRecentHistory();

    void loadFileList(const QStringList& paths);
    void applyProcessing(const std::vector<int>& indices, bool normalize, bool compress);
    void exportClips(const std::vector<int>& indices);
    std::vector<int> selectedIndices() const;
    std::vector<int> allIndices() const;
    AudioClip* currentClip();
    int currentClipIndex() const;
    void updateWaveformView();
    void updateTimeDisplay();

    // --- Data ---
    AudioEngine engine_;
    std::vector<AudioClip> clips_;

    // --- Settings ---
    QString lastOpenDirectory_;
    QStringList recentFiles_;
    QStringList recentFolders_;
    std::unique_ptr<QSettings> settings_;

    static constexpr int kMaxRecentItems = 10;

    // --- UI Components ---
    QTableView* clipTable_ = nullptr;
    ClipTableModel* clipModel_ = nullptr;
    QSortFilterProxyModel* proxyModel_ = nullptr;

    WaveformView* waveformView_ = nullptr;
    TransportPanel* transportPanel_ = nullptr;
    ProcessingPanel* processingPanel_ = nullptr;
    OutputPanel* outputPanel_ = nullptr;

    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;

    // Audio playback
    AudioPlayer* audioPlayer_ = nullptr;

    // Menus
    QMenu* recentFilesMenu_ = nullptr;
    QMenu* recentFoldersMenu_ = nullptr;

    // Menu actions
    QAction* openFilesAction_ = nullptr;
    QAction* openFolderAction_ = nullptr;
    QAction* settingsAction_ = nullptr;
    QAction* undoAction_ = nullptr;
    QAction* exitAction_ = nullptr;
};
