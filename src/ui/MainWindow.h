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
#include <memory>
#include <vector>

#include "audio/AudioClip.h"
#include "audio/AudioEngine.h"

class QTableView;
class QSortFilterProxyModel;
class QLabel;
class QProgressBar;
class QAction;
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
 *
 * Features:
 *  - Load audio files (WAV, MP3) individually or from a folder
 *  - Display clips in a sortable table
 *  - Preview clips with waveform visualization and playback
 *  - Trim clips visually using waveform markers
 *  - Apply normalize and compress processing (with undo)
 *  - Export processed clips to an output folder
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

    // Processing actions (from ProcessingPanel signals)
    void onApplyToSelected(bool normalize, bool compress);
    void onApplyToAll(bool normalize, bool compress);

    // Export actions (from OutputPanel signals)
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

private:
    void setupUi();
    void setupMenus();
    void loadSettings();
    void saveSettings();

    /**
     * @brief Load a list of file paths into the clip collection.
     * @param paths List of audio file paths.
     */
    void loadFileList(const QStringList& paths);

    /**
     * @brief Apply processing to a range of clips.
     * @param indices Row indices to process.
     * @param normalize Whether to apply normalization.
     * @param compress Whether to apply compression.
     */
    void applyProcessing(const std::vector<int>& indices, bool normalize, bool compress);

    /**
     * @brief Export clips to the output folder.
     * @param indices Row indices to export.
     */
    void exportClips(const std::vector<int>& indices);

    /**
     * @brief Get the currently selected row indices from the table.
     * @return Vector of selected row indices (in source model coordinates).
     */
    std::vector<int> selectedIndices() const;

    /**
     * @brief Get all row indices.
     * @return Vector of all row indices.
     */
    std::vector<int> allIndices() const;

    /**
     * @brief Get the currently focused clip (first selected).
     * @return Pointer to clip or nullptr if none selected.
     */
    AudioClip* currentClip();

    /**
     * @brief Get the index of the current clip.
     * @return Index or -1 if none selected.
     */
    int currentClipIndex() const;

    /**
     * @brief Update the waveform view with the current clip.
     */
    void updateWaveformView();

    /**
     * @brief Update the transport panel time display.
     */
    void updateTimeDisplay();

    // --- Data ---
    AudioEngine engine_;
    std::vector<AudioClip> clips_;

    // --- Settings ---
    QString lastOpenDirectory_;
    std::unique_ptr<QSettings> settings_;

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

    // Menu actions
    QAction* openFilesAction_ = nullptr;
    QAction* openFolderAction_ = nullptr;
    QAction* undoAction_ = nullptr;
    QAction* exitAction_ = nullptr;
};
