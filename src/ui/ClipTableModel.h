/**
 * @file ClipTableModel.h
 * @brief Table model for displaying audio clips with sortable columns.
 *
 * This model exposes clip metadata (name, duration, sample rate, channels,
 * peak dB, RMS dB, status) as columns in a QTableView. Works with QSortFilterProxyModel
 * for sorting support.
 */

#pragma once

#include <QAbstractTableModel>
#include <vector>
#include "audio/AudioClip.h"

class ProjectManager;

/**
 * @class ClipTableModel
 * @brief A QAbstractTableModel that displays AudioClip metadata in a table.
 *
 * Columns:
 *  0 - Name (file name without path)
 *  1 - Duration (seconds)
 *  2 - Sample Rate (Hz)
 *  3 - Channels
 *  4 - Peak (dBFS)
 *  5 - RMS (dB)
 *  6 - Status (processing state: T=Trimmed, N=Normalized, C=Compressed, E=Exported)
 */
class ClipTableModel : public QAbstractTableModel {
    Q_OBJECT

public:
    /// Column indices for the table
    enum Column {
        ColName = 0,
        ColDuration,
        ColSampleRate,
        ColChannels,
        ColPeakDb,
        ColRmsDb,
        ColStatus,
        ColCount  ///< Number of columns
    };

    /**
     * @brief Construct the model with a reference to the clips vector.
     * @param clips Reference to the vector of AudioClip objects.
     * @param projectManager Reference to the project manager for clip state queries.
     * @param parent Optional parent QObject.
     */
    explicit ClipTableModel(std::vector<AudioClip>& clips, 
                            ProjectManager& projectManager,
                            QObject* parent = nullptr);

    // --- QAbstractTableModel overrides ---

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Notify the view that the underlying clips data has changed.
     */
    void refresh();

    /**
     * @brief Get the AudioClip at a given row.
     */
    AudioClip* clipAt(int row);
    const AudioClip* clipAt(int row) const;

    /**
     * @brief Enable or disable column header tooltips.
     */
    void setShowTooltips(bool show);
    bool showTooltips() const;

private:
    std::vector<AudioClip>& clips_;
    ProjectManager& projectManager_;
    bool showTooltips_ = true;
};
