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
#include <QFutureWatcher>
#include <memory>
#include <vector>

#include "audio/AudioClip.h"
#include "audio/AudioEngine.h"
#include "core/ProjectManager.h"

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
class VuMeterWidget;

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
    void closeEvent(QCloseEvent* event) override;

private Q_SLOTS:
    // Project menu actions
    void newProject();
    void openProject();
    void saveProject();
    void saveProjectAs();
    void openRecentProject();
    void openProjectSettings();
    void openSettings();

    // Legacy file actions (kept for backward compatibility)
    void openFiles();
    void openFolder();
    void openRecentFile();
    void openRecentFolder();

    // Processing actions - normalize
    void onNormalizeSelected();
    void onNormalizeAll();
    
    // Processing actions - compress
    void onCompressSelected();
    void onCompressAll();

    // Edit mode changed
    void onEditModeChanged(bool isFadeMode);
    void onFadeChanged(int fadeInFrames, int fadeOutFrames);

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

    // Async loading/export completion
    void onLoadingFinished();
    void onExportFinished();
    void onProcessingFinished();

    // Project state changes
    void onProjectChanged();
    void onDirtyStateChanged(bool dirty);
    void onRecentProjectsChanged();

private:
    void setupUi();
    void setupMenus();
    void loadSettings();
    void saveSettings();

    // Project management
    void updateWindowTitle();
    void updateRecentProjectsMenu();
    bool maybeSaveProject();
    void loadProjectClips();
    void applyClipState(AudioClip& clip, const ClipState& state);

    // Recent files/folders management (legacy)
    void addRecentFile(const QString& path);
    void addRecentFolder(const QString& path);
    void updateRecentFilesMenu();
    void updateRecentFoldersMenu();
    void clearRecentHistory();

    void loadFileList(const QStringList& paths);
    void applyProcessing(const std::vector<int>& indices, bool normalize, bool compress);
    void exportClips(const std::vector<int>& indices);
    [[nodiscard]] std::vector<int> selectedIndices() const;
    [[nodiscard]] std::vector<int> allIndices() const;
    [[nodiscard]] AudioClip* currentClip();
    [[nodiscard]] int currentClipIndex() const;
    void updateWaveformView();
    void updateTimeDisplay();
    void refreshModelPreservingSelection();
    void selectRows(const std::vector<int>& indices);

    // --- Data ---
    AudioEngine engine_;
    std::vector<AudioClip> clips_;
    ProjectManager projectManager_;

    // --- Async operations ---
    QFutureWatcher<std::vector<AudioClip>>* loadWatcher_ = nullptr;
    QFutureWatcher<int>* exportWatcher_ = nullptr;
    QFutureWatcher<std::vector<AudioClip>>* processWatcher_ = nullptr;
    std::vector<int> processingIndices_;  // Tracks which indices were processed
    
    // Processing state for async completion
    bool processingAppliedNormalize_{false};
    bool processingAppliedCompress_{false};
    float processingNormalizeTarget_{0.0f};
    float processingCompThreshold_{0.0f};
    float processingCompRatio_{1.0f};
    float processingCompAttackMs_{10.0f};
    float processingCompReleaseMs_{100.0f};
    float processingCompMakeupDb_{0.0f};

    // --- Settings ---
    QString lastOpenDirectory_;
    QString defaultAuthorName_;
    QStringList recentFiles_;
    QStringList recentFolders_;
    std::unique_ptr<QSettings> settings_;
    bool showColumnTooltips_{true};

    static constexpr int kMaxRecentItems = 10;

    // --- UI Components ---
    QTableView* clipTable_ = nullptr;
    ClipTableModel* clipModel_ = nullptr;
    QSortFilterProxyModel* proxyModel_ = nullptr;

    WaveformView* waveformView_ = nullptr;
    TransportPanel* transportPanel_ = nullptr;
    ProcessingPanel* processingPanel_ = nullptr;
    OutputPanel* outputPanel_ = nullptr;
    VuMeterWidget* vuMeter_ = nullptr;

    QLabel* statusLabel_ = nullptr;
    QProgressBar* progressBar_ = nullptr;

    // Audio playback
    AudioPlayer* audioPlayer_ = nullptr;

    // Menus
    QMenu* recentFilesMenu_ = nullptr;
    QMenu* recentFoldersMenu_ = nullptr;
    QMenu* recentProjectsMenu_ = nullptr;

    // Menu actions
    QAction* newProjectAction_ = nullptr;
    QAction* openProjectAction_ = nullptr;
    QAction* saveProjectAction_ = nullptr;
    QAction* saveProjectAsAction_ = nullptr;
    QAction* projectSettingsAction_ = nullptr;
    QAction* openFilesAction_ = nullptr;
    QAction* openFolderAction_ = nullptr;
    QAction* settingsAction_ = nullptr;
    QAction* undoAction_ = nullptr;
    QAction* exitAction_ = nullptr;
};
