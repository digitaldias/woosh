/**
 * @file ClipTableModel.h
 * @brief Table model for displaying audio clips with sortable columns.
 *
 * This model exposes clip metadata (name, duration, sample rate, channels,
 * peak dB, RMS dB) as columns in a QTableView. Works with QSortFilterProxyModel
 * for sorting support.
 */

#pragma once

#include <QAbstractTableModel>
#include <vector>
#include "audio/AudioClip.h"

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
        ColCount  ///< Number of columns
    };

    /**
     * @brief Construct the model with a reference to the clips vector.
     * @param clips Reference to the vector of AudioClip objects.
     * @param parent Optional parent QObject.
     */
    explicit ClipTableModel(std::vector<AudioClip>& clips, QObject* parent = nullptr);

    // --- QAbstractTableModel overrides ---

    int rowCount(const QModelIndex& parent = QModelIndex()) const override;
    int columnCount(const QModelIndex& parent = QModelIndex()) const override;
    QVariant data(const QModelIndex& index, int role = Qt::DisplayRole) const override;
    QVariant headerData(int section, Qt::Orientation orientation, int role = Qt::DisplayRole) const override;

    /**
     * @brief Notify the view that the underlying clips data has changed.
     *
     * Call this after loading, processing, or removing clips to refresh the view.
     */
    void refresh();

    /**
     * @brief Get the AudioClip at a given row.
     * @param row The row index.
     * @return Pointer to the clip, or nullptr if out of range.
     */
    AudioClip* clipAt(int row);
    const AudioClip* clipAt(int row) const;

private:
    std::vector<AudioClip>& clips_;  ///< Reference to the clips collection
};

