/**
 * @file ClipTableModel.cpp
 * @brief Implementation of ClipTableModel for displaying audio clip metadata.
 */

#include "ClipTableModel.h"
#include <QFileInfo>

ClipTableModel::ClipTableModel(std::vector<AudioClip>& clips, QObject* parent)
    : QAbstractTableModel(parent)
    , clips_(clips)
{
}

int ClipTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;  // Flat table, no hierarchy
    return static_cast<int>(clips_.size());
}

int ClipTableModel::columnCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
    return ColCount;
}

QVariant ClipTableModel::data(const QModelIndex& index, int role) const {
    if (!index.isValid()) return {};
    if (index.row() < 0 || index.row() >= static_cast<int>(clips_.size())) return {};

    const auto& clip = clips_[static_cast<size_t>(index.row())];

    if (role == Qt::DisplayRole) {
        switch (index.column()) {
            case ColName:
                return QString::fromStdString(clip.displayName());
            case ColDuration:
                return QString::number(clip.durationSeconds(), 'f', 2);
            case ColSampleRate:
                return QString::number(clip.sampleRate());
            case ColChannels:
                return QString::number(clip.channels());
            case ColPeakDb:
                return QString::number(clip.peakDb(), 'f', 2);
            case ColRmsDb:
                return QString::number(clip.rmsDb(), 'f', 2);
            default:
                return {};
        }
    }

    // For sorting: return raw numeric values
    if (role == Qt::UserRole) {
        switch (index.column()) {
            case ColName:
                return QString::fromStdString(clip.displayName());
            case ColDuration:
                return clip.durationSeconds();
            case ColSampleRate:
                return clip.sampleRate();
            case ColChannels:
                return clip.channels();
            case ColPeakDb:
                return static_cast<double>(clip.peakDb());
            case ColRmsDb:
                return static_cast<double>(clip.rmsDb());
            default:
                return {};
        }
    }

    // Text alignment: right-align numeric columns
    if (role == Qt::TextAlignmentRole) {
        if (index.column() != ColName) {
            return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
    }

    return {};
}

QVariant ClipTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal || role != Qt::DisplayRole) {
        return {};
    }

    switch (section) {
        case ColName:       return tr("Name");
        case ColDuration:   return tr("Duration (s)");
        case ColSampleRate: return tr("Sample Rate");
        case ColChannels:   return tr("Ch");
        case ColPeakDb:     return tr("Peak dB");
        case ColRmsDb:      return tr("RMS dB");
        default:            return {};
    }
}

void ClipTableModel::refresh() {
    beginResetModel();
    endResetModel();
}

AudioClip* ClipTableModel::clipAt(int row) {
    if (row < 0 || row >= static_cast<int>(clips_.size())) return nullptr;
    return &clips_[static_cast<size_t>(row)];
}

const AudioClip* ClipTableModel::clipAt(int row) const {
    if (row < 0 || row >= static_cast<int>(clips_.size())) return nullptr;
    return &clips_[static_cast<size_t>(row)];
}

