/**
 * @file ClipTableModel.cpp
 * @brief Implementation of ClipTableModel for displaying audio clip metadata.
 */

#include "ClipTableModel.h"
#include "core/ProjectManager.h"
#include <QFileInfo>
#include <QSettings>
#include <QBrush>
#include <QColor>
#include <filesystem>

namespace {
    constexpr const char* kSettingsOrg = "Woosh";
    constexpr const char* kSettingsApp = "WooshEditor";
    constexpr const char* kKeyShowTooltips = "UI/ShowColumnTooltips";
}

ClipTableModel::ClipTableModel(std::vector<AudioClip>& clips, 
                               ProjectManager& projectManager,
                               QObject* parent)
    : QAbstractTableModel(parent)
    , clips_(clips)
    , projectManager_(projectManager)
{
    // Load tooltip preference
    QSettings settings(kSettingsOrg, kSettingsApp);
    showTooltips_ = settings.value(kKeyShowTooltips, true).toBool();
}

int ClipTableModel::rowCount(const QModelIndex& parent) const {
    if (parent.isValid()) return 0;
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
    
    // Get clip state from project
    const ClipState* clipState = nullptr;
    if (projectManager_.hasProject()) {
        // Build relative path from clip path
        const std::string& clipPath = clip.filePath();
        const std::string& rawFolder = projectManager_.project().rawFolder();
        std::string relativePath;
        
        if (!rawFolder.empty()) {
            std::filesystem::path clipFsPath(clipPath);
            std::filesystem::path rawFsPath(rawFolder);
            auto relPath = std::filesystem::relative(clipFsPath, rawFsPath);
            relativePath = relPath.string();
        } else {
            relativePath = std::filesystem::path(clipPath).filename().string();
        }
        
        clipState = projectManager_.project().findClipState(relativePath);
    }

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
            case ColStatus: {
                if (!clipState) return QString();
                QString status;
                if (clipState->isTrimmed) status += "T";
                if (clipState->isNormalized) status += "N";
                if (clipState->isCompressed) status += "C";
                if (clipState->isExported) status += "E";
                return status;
            }
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
            case ColStatus: {
                // Sort by number of operations applied
                if (!clipState) return 0;
                int count = 0;
                if (clipState->isTrimmed) count++;
                if (clipState->isNormalized) count++;
                if (clipState->isCompressed) count++;
                if (clipState->isExported) count++;
                return count;
            }
            default:
                return {};
        }
    }

    // Text alignment: right-align numeric columns
    if (role == Qt::TextAlignmentRole) {
        if (index.column() == ColName) {
            return static_cast<int>(Qt::AlignLeft | Qt::AlignVCenter);
        }
        if (index.column() == ColStatus) {
            return static_cast<int>(Qt::AlignCenter | Qt::AlignVCenter);
        }
        return static_cast<int>(Qt::AlignRight | Qt::AlignVCenter);
    }
    
    // Tooltip for status column
    if (role == Qt::ToolTipRole && index.column() == ColStatus) {
        if (!clipState) {
            return tr("No processing applied");
        }
        
        QStringList operations;
        
        if (clipState->isTrimmed) {
            operations << tr("Trimmed: %1s - %2s")
                          .arg(clipState->trimStartSec, 0, 'f', 3)
                          .arg(clipState->trimEndSec, 0, 'f', 3);
        }
        
        if (clipState->isNormalized) {
            operations << tr("Normalized: %1 dB")
                          .arg(clipState->normalizeTargetDb, 0, 'f', 1);
        }
        
        if (clipState->isCompressed) {
            const auto& cs = clipState->compressorSettings;
            operations << tr("Compressed: %1 dB, %2:1, %3/%4ms, +%5 dB")
                          .arg(cs.threshold, 0, 'f', 1)
                          .arg(cs.ratio, 0, 'f', 1)
                          .arg(cs.attackMs, 0, 'f', 0)
                          .arg(cs.releaseMs, 0, 'f', 0)
                          .arg(cs.makeupDb, 0, 'f', 1);
        }
        
        if (clipState->isExported) {
            operations << tr("Exported: %1")
                          .arg(QString::fromStdString(clipState->exportedFilename));
        }
        
        if (operations.isEmpty()) {
            return tr("No processing applied");
        }
        
        return operations.join("\n");
    }
    
    // Background color for modified rows
    if (role == Qt::BackgroundRole) {
        if (clipState) {
            bool hasModifications = clipState->isTrimmed || clipState->isNormalized || 
                                   clipState->isCompressed;
            bool isExported = clipState->isExported;
            
            if (isExported) {
                // Light green for exported clips
                return QBrush(QColor(200, 255, 200, 50));
            } else if (hasModifications) {
                // Light yellow for modified but not exported
                return QBrush(QColor(255, 255, 200, 50));
            }
        }
        return {};
    }

    return {};
}

QVariant ClipTableModel::headerData(int section, Qt::Orientation orientation, int role) const {
    if (orientation != Qt::Horizontal) {
        return {};
    }

    if (role == Qt::DisplayRole) {
        switch (section) {
            case ColName:       return tr("Name");
            case ColDuration:   return tr("Duration (s)");
            case ColSampleRate: return tr("Sample Rate");
            case ColChannels:   return tr("Ch");
            case ColPeakDb:     return tr("Peak dB");
            case ColRmsDb:      return tr("RMS dB");
            case ColStatus:     return tr("Status");
            default:            return {};
        }
    }

    // Column tooltips
    if (role == Qt::ToolTipRole && showTooltips_) {
        switch (section) {
            case ColName:
                return tr("The filename of the audio clip");
            case ColDuration:
                return tr("Duration of the clip in seconds");
            case ColSampleRate:
                return tr("Sample rate in Hz (samples per second).\n"
                          "Common rates: 44100 Hz (CD quality), 48000 Hz (video/broadcast)");
            case ColChannels:
                return tr("Number of audio channels.\n"
                          "1 = Mono, 2 = Stereo");
            case ColPeakDb:
                return tr("Peak Level (dBFS)\n\n"
                          "The loudest sample in the clip measured in decibels\n"
                          "relative to full scale. 0 dB = maximum digital level.\n"
                          "Negative values indicate headroom before clipping.");
            case ColRmsDb:
                return tr("RMS Level (dB)\n\n"
                          "Root Mean Square - measures the average loudness\n"
                          "of the audio over time. More representative of\n"
                          "perceived loudness than peak level.\n"
                          "Typical values: -20 to -10 dB for normal audio.");
            case ColStatus:
                return tr("Processing Status\n\n"
                          "Shows which operations have been applied:\n"
                          "  T = Trimmed\n"
                          "  N = Normalized\n"
                          "  C = Compressed\n"
                          "  E = Exported\n\n"
                          "Hover over a status to see detailed parameters.");
            default:
                return {};
        }
    }

    return {};
}

void ClipTableModel::refresh() {
    beginResetModel();
    endResetModel();
}

void ClipTableModel::setShowTooltips(bool show) {
    showTooltips_ = show;
    // Save preference
    QSettings settings(kSettingsOrg, kSettingsApp);
    settings.setValue(kKeyShowTooltips, show);
    // Notify header to update
    Q_EMIT headerDataChanged(Qt::Horizontal, 0, ColCount - 1);
}

bool ClipTableModel::showTooltips() const {
    return showTooltips_;
}

AudioClip* ClipTableModel::clipAt(int row) {
    if (row < 0 || row >= static_cast<int>(clips_.size())) return nullptr;
    return &clips_[static_cast<size_t>(row)];
}

const AudioClip* ClipTableModel::clipAt(int row) const {
    if (row < 0 || row >= static_cast<int>(clips_.size())) return nullptr;
    return &clips_[static_cast<size_t>(row)];
}
