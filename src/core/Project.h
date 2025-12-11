/**
 * @file Project.h
 * @brief Woosh project file model for game audio batch processing.
 *
 * A project encapsulates:
 * - RAW audio input folder
 * - Game audio output folder
 * - Per-clip processing state (normalized, compressed, trimmed, exported)
 * - Export settings (format, bitrate, metadata)
 * - Processing settings defaults
 */

#pragma once

#include <string>
#include <vector>
#include <optional>
#include <functional>

/**
 * @brief Export format options.
 */
enum class ExportFormat {
    MP3,
    OGG,
    WAV
};

/**
 * @brief Compressor settings applied to a clip.
 */
struct CompressorSettings {
    float threshold{0.0f};    ///< Threshold in dB
    float ratio{1.0f};        ///< Compression ratio (e.g., 4.0 = 4:1)
    float attackMs{10.0f};    ///< Attack time in milliseconds
    float releaseMs{100.0f};  ///< Release time in milliseconds
    float makeupDb{0.0f};     ///< Makeup gain in dB
};

/**
 * @brief Per-clip processing state tracking.
 *
 * Tracks what operations have been applied to each audio clip
 * and the parameters used.
 */
struct ClipState {
    std::string relativePath;           ///< Path relative to RAW folder
    
    // Processing flags
    bool isNormalized{false};
    bool isCompressed{false};
    bool isTrimmed{false};
    bool isExported{false};
    
    // Normalization parameters (if applied)
    double normalizeTargetDb{0.0};
    
    // Compressor parameters (if applied)
    CompressorSettings compressorSettings;
    
    // Trim parameters (if applied)
    double trimStartSec{0.0};
    double trimEndSec{0.0};
    
    // Fade parameters (in frames)
    int fadeInFrames{0};
    int fadeOutFrames{0};
    
    // Export info
    std::string exportedFilename;       ///< Name of exported file in game folder
};

/**
 * @brief Project-wide export settings.
 */
struct ExportSettings {
    ExportFormat format{ExportFormat::MP3};
    int mp3Bitrate{192};                ///< MP3 bitrate: 128, 160, or 192 kbps
    std::string gameName;               ///< Game name for metadata
    std::string authorName;             ///< Author/studio name for metadata
    bool embedMetadata{true};           ///< Whether to embed ID3/Vorbis tags
};

/**
 * @brief Default processing settings for new operations.
 */
struct ProcessingSettings {
    double normalizeTargetDb{-1.0};     ///< Default normalize target
    float compThreshold{-12.0f};        ///< Default compressor threshold
    float compRatio{4.0f};              ///< Default compressor ratio
    float compAttackMs{10.0f};          ///< Default attack time
    float compReleaseMs{100.0f};        ///< Default release time
    float compMakeupDb{0.0f};           ///< Default makeup gain
};

/**
 * @class Project
 * @brief Represents a Woosh project file (.wooshp).
 *
 * Manages project state including folders, clip states, and settings.
 * Supports JSON serialization for persistence.
 */
class Project final {
public:
    static constexpr int kCurrentVersion = 1;
    static constexpr const char* kFileExtension = ".wooshp";
    
    Project() = default;
    
    // --- Accessors ---
    
    [[nodiscard]] const std::string& name() const noexcept { return name_; }
    [[nodiscard]] const std::string& rawFolder() const noexcept { return rawFolder_; }
    [[nodiscard]] const std::string& gameFolder() const noexcept { return gameFolder_; }
    [[nodiscard]] const std::string& filePath() const noexcept { return filePath_; }
    [[nodiscard]] const std::vector<ClipState>& clipStates() const noexcept { return clipStates_; }
    [[nodiscard]] const ExportSettings& exportSettings() const noexcept { return exportSettings_; }
    [[nodiscard]] const ProcessingSettings& processingSettings() const noexcept { return processingSettings_; }
    [[nodiscard]] bool isDirty() const noexcept { return dirty_; }
    
    // --- Mutators ---
    
    void setName(const std::string& name);
    void setRawFolder(const std::string& path);
    void setGameFolder(const std::string& path);
    void setExportSettings(const ExportSettings& settings);
    void setProcessingSettings(const ProcessingSettings& settings);
    
    // --- Clip state management ---
    
    void addClipState(const ClipState& state);
    void updateClipState(const std::string& relativePath, 
                         const std::function<void(ClipState&)>& updater);
    [[nodiscard]] ClipState* findClipState(const std::string& relativePath);
    [[nodiscard]] const ClipState* findClipState(const std::string& relativePath) const;
    bool removeClipState(const std::string& relativePath);
    void clearClipStates();
    
    // --- Dirty state ---
    
    void clearDirty() noexcept { dirty_ = false; }
    void markDirty() noexcept { dirty_ = true; }
    
    // --- Serialization ---
    
    /**
     * @brief Save project to file.
     * @param path File path (should end in .wooshp)
     * @return True if save succeeded
     */
    [[nodiscard]] bool save(const std::string& path);
    
    /**
     * @brief Load project from file.
     * @param path File path to .wooshp file
     * @return Loaded project or nullopt on failure
     */
    [[nodiscard]] static std::optional<Project> load(const std::string& path);
    
private:
    std::string name_;
    std::string rawFolder_;
    std::string gameFolder_;
    std::string filePath_;              ///< Path where project was saved/loaded
    
    std::vector<ClipState> clipStates_;
    ExportSettings exportSettings_;
    ProcessingSettings processingSettings_;
    
    bool dirty_{false};
};
