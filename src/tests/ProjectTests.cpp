/**
 * @file ProjectTests.cpp
 * @brief Unit tests for Project and ClipState classes.
 */

#include <cassert>
#include <cmath>
#include <string>
#include <fstream>
#include <sstream>
#include <filesystem>
#include "core/Project.h"

// ============================================================================
// Helper functions
// ============================================================================

static bool approxEqual(double a, double b, double tolerance = 0.001) {
    return std::abs(a - b) < tolerance;
}

static std::string getTempProjectPath() {
    return (std::filesystem::temp_directory_path() / "test_project.wooshp").string();
}

static void cleanupTempFile(const std::string& path) {
    std::filesystem::remove(path);
}

// ============================================================================
// ClipState tests
// ============================================================================

static void testClipState_defaultConstruction() {
    ClipState state;
    assert(state.relativePath.empty());
    assert(!state.isNormalized);
    assert(!state.isCompressed);
    assert(!state.isTrimmed);
    assert(!state.isExported);
    assert(state.normalizeTargetDb == 0.0);
    assert(state.compressorSettings.threshold == 0.0f);
}

static void testClipState_setNormalized() {
    ClipState state;
    state.relativePath = "audio/test.wav";
    state.isNormalized = true;
    state.normalizeTargetDb = -1.0;
    
    assert(state.isNormalized);
    assert(approxEqual(state.normalizeTargetDb, -1.0));
}

static void testClipState_setCompressed() {
    ClipState state;
    state.isCompressed = true;
    state.compressorSettings.threshold = -12.0f;
    state.compressorSettings.ratio = 4.0f;
    state.compressorSettings.attackMs = 10.0f;
    state.compressorSettings.releaseMs = 100.0f;
    state.compressorSettings.makeupDb = 3.0f;
    
    assert(state.isCompressed);
    assert(approxEqual(state.compressorSettings.threshold, -12.0f, 0.1f));
    assert(approxEqual(state.compressorSettings.ratio, 4.0f, 0.1f));
}

static void testClipState_setTrimmed() {
    ClipState state;
    state.isTrimmed = true;
    state.trimStartSec = 0.5;
    state.trimEndSec = 2.5;
    
    assert(state.isTrimmed);
    assert(approxEqual(state.trimStartSec, 0.5));
    assert(approxEqual(state.trimEndSec, 2.5));
}

static void testClipState_setExported() {
    ClipState state;
    state.isExported = true;
    state.exportedFilename = "test_processed.mp3";
    
    assert(state.isExported);
    assert(state.exportedFilename == "test_processed.mp3");
}

// ============================================================================
// ClipState status badge generation tests
// ============================================================================

// Helper function that mirrors the logic in ClipTableModel::data() for status badges
static std::string buildStatusBadge(const ClipState& state) {
    std::string status;
    if (state.isTrimmed) status += "T";
    if (state.isNormalized) status += "N";
    if (state.isCompressed) status += "C";
    if (state.isExported) status += "E";
    return status;
}

// Helper function that mirrors the logic in ClipTableModel::data() for sort value
static int countOperations(const ClipState& state) {
    int count = 0;
    if (state.isTrimmed) count++;
    if (state.isNormalized) count++;
    if (state.isCompressed) count++;
    if (state.isExported) count++;
    return count;
}

static void testClipState_statusBadge_noOperations() {
    ClipState state;
    state.relativePath = "test.wav";
    
    assert(buildStatusBadge(state).empty());
    assert(countOperations(state) == 0);
}

static void testClipState_statusBadge_trimmedOnly() {
    ClipState state;
    state.isTrimmed = true;
    
    assert(buildStatusBadge(state) == "T");
    assert(countOperations(state) == 1);
}

static void testClipState_statusBadge_normalizedOnly() {
    ClipState state;
    state.isNormalized = true;
    
    assert(buildStatusBadge(state) == "N");
    assert(countOperations(state) == 1);
}

static void testClipState_statusBadge_compressedOnly() {
    ClipState state;
    state.isCompressed = true;
    
    assert(buildStatusBadge(state) == "C");
    assert(countOperations(state) == 1);
}

static void testClipState_statusBadge_exportedOnly() {
    ClipState state;
    state.isExported = true;
    
    assert(buildStatusBadge(state) == "E");
    assert(countOperations(state) == 1);
}

static void testClipState_statusBadge_multipleOperations() {
    ClipState state;
    state.isTrimmed = true;
    state.isNormalized = true;
    state.isCompressed = true;
    
    // Order should be T, N, C, E
    assert(buildStatusBadge(state) == "TNC");
    assert(countOperations(state) == 3);
}

static void testClipState_statusBadge_allOperations() {
    ClipState state;
    state.isTrimmed = true;
    state.isNormalized = true;
    state.isCompressed = true;
    state.isExported = true;
    
    assert(buildStatusBadge(state) == "TNCE");
    assert(countOperations(state) == 4);
}

static void testClipState_statusBadge_commonWorkflow_normalizeAndExport() {
    // Common workflow: normalize then export
    ClipState state;
    state.isNormalized = true;
    state.normalizeTargetDb = -1.0;
    state.isExported = true;
    state.exportedFilename = "output.mp3";
    
    assert(buildStatusBadge(state) == "NE");
    assert(countOperations(state) == 2);
}

static void testClipState_statusBadge_commonWorkflow_fullProcessing() {
    // Full processing workflow: trim, normalize, compress, export
    ClipState state;
    state.isTrimmed = true;
    state.trimStartSec = 0.1;
    state.trimEndSec = 2.5;
    state.isNormalized = true;
    state.normalizeTargetDb = -1.0;
    state.isCompressed = true;
    state.compressorSettings.threshold = -12.0f;
    state.compressorSettings.ratio = 4.0f;
    state.isExported = true;
    state.exportedFilename = "processed.mp3";
    
    assert(buildStatusBadge(state) == "TNCE");
    assert(countOperations(state) == 4);
}

// ============================================================================
// ExportSettings tests
// ============================================================================

static void testExportSettings_defaultConstruction() {
    ExportSettings settings;
    assert(settings.format == ExportFormat::MP3);
    assert(settings.mp3Bitrate == 192);
    assert(settings.gameName.empty());
    assert(settings.authorName.empty());
    assert(settings.embedMetadata);
}

static void testExportSettings_customValues() {
    ExportSettings settings;
    settings.format = ExportFormat::OGG;
    settings.mp3Bitrate = 128;
    settings.gameName = "My Awesome Game";
    settings.authorName = "Game Studio";
    settings.embedMetadata = true;
    
    assert(settings.format == ExportFormat::OGG);
    assert(settings.mp3Bitrate == 128);
    assert(settings.gameName == "My Awesome Game");
}

// ============================================================================
// ProcessingSettings tests
// ============================================================================

static void testProcessingSettings_defaultConstruction() {
    ProcessingSettings settings;
    assert(approxEqual(settings.normalizeTargetDb, -1.0));
    assert(approxEqual(settings.compThreshold, -12.0f, 0.1f));
    assert(approxEqual(settings.compRatio, 4.0f, 0.1f));
    assert(approxEqual(settings.compAttackMs, 10.0f, 0.1f));
    assert(approxEqual(settings.compReleaseMs, 100.0f, 0.1f));
    assert(approxEqual(settings.compMakeupDb, 0.0f, 0.1f));
}

// ============================================================================
// Project construction tests
// ============================================================================

static void testProject_defaultConstruction() {
    Project project;
    assert(project.name().empty());
    assert(project.rawFolder().empty());
    assert(project.gameFolder().empty());
    assert(project.clipStates().empty());
    assert(!project.isDirty());
}

static void testProject_setName() {
    Project project;
    project.setName("My Game Audio");
    
    assert(project.name() == "My Game Audio");
    assert(project.isDirty());
}

static void testProject_setRawFolder() {
    Project project;
    project.setRawFolder("/path/to/raw/audio");
    
    assert(project.rawFolder() == "/path/to/raw/audio");
    assert(project.isDirty());
}

static void testProject_setGameFolder() {
    Project project;
    project.setGameFolder("/path/to/game/sounds");
    
    assert(project.gameFolder() == "/path/to/game/sounds");
    assert(project.isDirty());
}

static void testProject_setExportSettings() {
    Project project;
    ExportSettings settings;
    settings.format = ExportFormat::MP3;
    settings.mp3Bitrate = 128;
    settings.gameName = "Test Game";
    
    project.setExportSettings(settings);
    
    assert(project.exportSettings().format == ExportFormat::MP3);
    assert(project.exportSettings().mp3Bitrate == 128);
    assert(project.exportSettings().gameName == "Test Game");
    assert(project.isDirty());
}

// ============================================================================
// Clip state management tests
// ============================================================================

static void testProject_addClipState() {
    Project project;
    ClipState state;
    state.relativePath = "sounds/explosion.wav";
    
    project.addClipState(state);
    
    assert(project.clipStates().size() == 1);
    assert(project.clipStates()[0].relativePath == "sounds/explosion.wav");
    assert(project.isDirty());
}

static void testProject_updateClipState() {
    Project project;
    ClipState state;
    state.relativePath = "sounds/explosion.wav";
    project.addClipState(state);
    project.clearDirty();
    
    // Update the clip state
    project.updateClipState("sounds/explosion.wav", [](ClipState& s) {
        s.isNormalized = true;
        s.normalizeTargetDb = -3.0;
    });
    
    assert(project.clipStates()[0].isNormalized);
    assert(approxEqual(project.clipStates()[0].normalizeTargetDb, -3.0));
    assert(project.isDirty());
}

static void testProject_updateClipState_withNormalization() {
    Project project;
    ClipState state;
    state.relativePath = "audio/test.wav";
    project.addClipState(state);
    project.clearDirty();
    
    // Simulate normalization being applied (as MainWindow::onProcessingFinished does)
    float normTarget = -1.0f;
    project.updateClipState("audio/test.wav", [normTarget](ClipState& s) {
        s.isNormalized = true;
        s.normalizeTargetDb = static_cast<double>(normTarget);
    });
    
    auto* found = project.findClipState("audio/test.wav");
    assert(found != nullptr);
    assert(found->isNormalized);
    assert(approxEqual(found->normalizeTargetDb, -1.0));
    assert(!found->isCompressed);  // Should remain unchanged
    assert(!found->isTrimmed);     // Should remain unchanged
}

static void testProject_updateClipState_withCompression() {
    Project project;
    ClipState state;
    state.relativePath = "audio/test.wav";
    project.addClipState(state);
    project.clearDirty();
    
    // Simulate compression being applied (as MainWindow::onProcessingFinished does)
    float threshold = -12.0f;
    float ratio = 4.0f;
    float attack = 10.0f;
    float release = 100.0f;
    float makeup = 3.0f;
    
    project.updateClipState("audio/test.wav", 
        [threshold, ratio, attack, release, makeup](ClipState& s) {
            s.isCompressed = true;
            s.compressorSettings.threshold = threshold;
            s.compressorSettings.ratio = ratio;
            s.compressorSettings.attackMs = attack;
            s.compressorSettings.releaseMs = release;
            s.compressorSettings.makeupDb = makeup;
        });
    
    auto* found = project.findClipState("audio/test.wav");
    assert(found != nullptr);
    assert(found->isCompressed);
    assert(approxEqual(found->compressorSettings.threshold, -12.0f, 0.1f));
    assert(approxEqual(found->compressorSettings.ratio, 4.0f, 0.1f));
    assert(approxEqual(found->compressorSettings.attackMs, 10.0f, 0.1f));
    assert(approxEqual(found->compressorSettings.releaseMs, 100.0f, 0.1f));
    assert(approxEqual(found->compressorSettings.makeupDb, 3.0f, 0.1f));
}

static void testProject_updateClipState_withTrim() {
    Project project;
    ClipState state;
    state.relativePath = "audio/test.wav";
    project.addClipState(state);
    project.clearDirty();
    
    // Simulate trim being applied (as MainWindow::onApplyTrim does)
    double trimStartSec = 0.5;
    double trimEndSec = 2.5;
    
    project.updateClipState("audio/test.wav", 
        [trimStartSec, trimEndSec](ClipState& s) {
            s.isTrimmed = true;
            s.trimStartSec = trimStartSec;
            s.trimEndSec = trimEndSec;
        });
    
    auto* found = project.findClipState("audio/test.wav");
    assert(found != nullptr);
    assert(found->isTrimmed);
    assert(approxEqual(found->trimStartSec, 0.5));
    assert(approxEqual(found->trimEndSec, 2.5));
}

static void testProject_updateClipState_multipleOperations() {
    Project project;
    ClipState state;
    state.relativePath = "audio/test.wav";
    project.addClipState(state);
    
    // Apply normalize
    project.updateClipState("audio/test.wav", [](ClipState& s) {
        s.isNormalized = true;
        s.normalizeTargetDb = -1.0;
    });
    
    // Apply compress (should preserve normalization state)
    project.updateClipState("audio/test.wav", [](ClipState& s) {
        s.isCompressed = true;
        s.compressorSettings.threshold = -12.0f;
        s.compressorSettings.ratio = 4.0f;
    });
    
    // Apply trim (should preserve both normalize and compress state)
    project.updateClipState("audio/test.wav", [](ClipState& s) {
        s.isTrimmed = true;
        s.trimStartSec = 0.1;
        s.trimEndSec = 1.5;
    });
    
    auto* found = project.findClipState("audio/test.wav");
    assert(found != nullptr);
    // All three operations should be recorded
    assert(found->isNormalized);
    assert(found->isCompressed);
    assert(found->isTrimmed);
    assert(approxEqual(found->normalizeTargetDb, -1.0));
    assert(approxEqual(found->compressorSettings.threshold, -12.0f, 0.1f));
    assert(approxEqual(found->trimStartSec, 0.1));
}

static void testProject_updateClipState_nonexistentClip() {
    Project project;
    ClipState state;
    state.relativePath = "audio/exists.wav";
    project.addClipState(state);
    project.clearDirty();
    
    // Try to update a clip that doesn't exist
    project.updateClipState("audio/nonexistent.wav", [](ClipState& s) {
        s.isNormalized = true;
    });
    
    // The existing clip should be unchanged
    auto* found = project.findClipState("audio/exists.wav");
    assert(found != nullptr);
    assert(!found->isNormalized);
}

static void testProject_findClipState() {
    Project project;
    ClipState state1;
    state1.relativePath = "sounds/explosion.wav";
    ClipState state2;
    state2.relativePath = "sounds/footstep.wav";
    
    project.addClipState(state1);
    project.addClipState(state2);
    
    auto* found = project.findClipState("sounds/footstep.wav");
    assert(found != nullptr);
    assert(found->relativePath == "sounds/footstep.wav");
    
    auto* notFound = project.findClipState("sounds/nonexistent.wav");
    assert(notFound == nullptr);
}

static void testProject_removeClipState() {
    Project project;
    ClipState state;
    state.relativePath = "sounds/explosion.wav";
    project.addClipState(state);
    project.clearDirty();
    
    bool removed = project.removeClipState("sounds/explosion.wav");
    
    assert(removed);
    assert(project.clipStates().empty());
    assert(project.isDirty());
}

static void testProject_clearClipStates() {
    Project project;
    ClipState state1, state2;
    state1.relativePath = "a.wav";
    state2.relativePath = "b.wav";
    project.addClipState(state1);
    project.addClipState(state2);
    project.clearDirty();
    
    project.clearClipStates();
    
    assert(project.clipStates().empty());
    assert(project.isDirty());
}

// ============================================================================
// Dirty state tests
// ============================================================================

static void testProject_dirtyState_initiallyClean() {
    Project project;
    assert(!project.isDirty());
}

static void testProject_dirtyState_afterModification() {
    Project project;
    project.setName("Test");
    assert(project.isDirty());
}

static void testProject_dirtyState_clearDirty() {
    Project project;
    project.setName("Test");
    assert(project.isDirty());
    
    project.clearDirty();
    assert(!project.isDirty());
}

static void testProject_dirtyState_afterSave() {
    Project project;
    project.setName("Test");
    project.setRawFolder("/raw");
    project.setGameFolder("/game");
    
    std::string path = getTempProjectPath();
    bool saved = project.save(path);
    
    assert(saved);
    assert(!project.isDirty()); // Save should clear dirty
    
    cleanupTempFile(path);
}

// ============================================================================
// Serialization tests
// ============================================================================

static void testProject_saveAndLoad_basicProperties() {
    Project original;
    original.setName("Test Project");
    original.setRawFolder("/path/to/raw");
    original.setGameFolder("/path/to/game");
    
    std::string path = getTempProjectPath();
    bool saved = original.save(path);
    assert(saved);
    
    auto loaded = Project::load(path);
    assert(loaded.has_value());
    assert(loaded->name() == "Test Project");
    assert(loaded->rawFolder() == "/path/to/raw");
    assert(loaded->gameFolder() == "/path/to/game");
    assert(!loaded->isDirty()); // Freshly loaded should be clean
    
    cleanupTempFile(path);
}

static void testProject_saveAndLoad_exportSettings() {
    Project original;
    original.setName("Export Test");
    original.setRawFolder("/raw");
    original.setGameFolder("/game");
    
    ExportSettings settings;
    settings.format = ExportFormat::MP3;
    settings.mp3Bitrate = 160;
    settings.gameName = "Super Game";
    settings.authorName = "Cool Developer";
    settings.embedMetadata = true;
    original.setExportSettings(settings);
    
    std::string path = getTempProjectPath();
    original.save(path);
    
    auto loaded = Project::load(path);
    assert(loaded.has_value());
    assert(loaded->exportSettings().format == ExportFormat::MP3);
    assert(loaded->exportSettings().mp3Bitrate == 160);
    assert(loaded->exportSettings().gameName == "Super Game");
    assert(loaded->exportSettings().authorName == "Cool Developer");
    
    cleanupTempFile(path);
}

static void testProject_saveAndLoad_processingSettings() {
    Project original;
    original.setName("Processing Test");
    original.setRawFolder("/raw");
    original.setGameFolder("/game");
    
    ProcessingSettings procSettings;
    procSettings.normalizeTargetDb = -3.0;
    procSettings.compThreshold = -18.0f;
    procSettings.compRatio = 6.0f;
    procSettings.compAttackMs = 5.0f;
    procSettings.compReleaseMs = 150.0f;
    procSettings.compMakeupDb = 4.0f;
    original.setProcessingSettings(procSettings);
    
    std::string path = getTempProjectPath();
    original.save(path);
    
    auto loaded = Project::load(path);
    assert(loaded.has_value());
    assert(approxEqual(loaded->processingSettings().normalizeTargetDb, -3.0));
    assert(approxEqual(loaded->processingSettings().compThreshold, -18.0f, 0.1f));
    assert(approxEqual(loaded->processingSettings().compRatio, 6.0f, 0.1f));
    
    cleanupTempFile(path);
}

static void testProject_saveAndLoad_clipStates() {
    Project original;
    original.setName("Clips Test");
    original.setRawFolder("/raw");
    original.setGameFolder("/game");
    
    ClipState clip1;
    clip1.relativePath = "sounds/boom.wav";
    clip1.isNormalized = true;
    clip1.normalizeTargetDb = -1.0;
    clip1.isCompressed = true;
    clip1.compressorSettings.threshold = -12.0f;
    clip1.compressorSettings.ratio = 4.0f;
    clip1.isExported = true;
    clip1.exportedFilename = "boom.mp3";
    
    ClipState clip2;
    clip2.relativePath = "music/theme.wav";
    clip2.isTrimmed = true;
    clip2.trimStartSec = 1.0;
    clip2.trimEndSec = 30.0;
    
    original.addClipState(clip1);
    original.addClipState(clip2);
    
    std::string path = getTempProjectPath();
    original.save(path);
    
    auto loaded = Project::load(path);
    assert(loaded.has_value());
    assert(loaded->clipStates().size() == 2);
    
    auto* loadedClip1 = loaded->findClipState("sounds/boom.wav");
    assert(loadedClip1 != nullptr);
    assert(loadedClip1->isNormalized);
    assert(loadedClip1->isCompressed);
    assert(loadedClip1->isExported);
    assert(loadedClip1->exportedFilename == "boom.mp3");
    
    auto* loadedClip2 = loaded->findClipState("music/theme.wav");
    assert(loadedClip2 != nullptr);
    assert(loadedClip2->isTrimmed);
    assert(approxEqual(loadedClip2->trimStartSec, 1.0));
    assert(approxEqual(loadedClip2->trimEndSec, 30.0));
    
    cleanupTempFile(path);
}

static void testProject_load_nonexistentFile() {
    auto result = Project::load("/nonexistent/path/project.wooshp");
    assert(!result.has_value());
}

static void testProject_load_invalidJson() {
    // Create a file with invalid JSON
    std::string path = getTempProjectPath();
    {
        std::ofstream file(path);
        file << "{ invalid json content }}}";
    }
    
    auto result = Project::load(path);
    assert(!result.has_value());
    
    cleanupTempFile(path);
}

static void testProject_load_missingRequiredFields() {
    // Create a file with valid JSON but missing required fields
    std::string path = getTempProjectPath();
    {
        std::ofstream file(path);
        file << R"({"version": 1})";
    }
    
    auto result = Project::load(path);
    // Should still load but with empty strings for missing fields
    assert(result.has_value());
    assert(result->name().empty());
    
    cleanupTempFile(path);
}

// ============================================================================
// Version compatibility tests
// ============================================================================

static void testProject_versionField() {
    Project project;
    project.setName("Version Test");
    project.setRawFolder("/raw");
    project.setGameFolder("/game");
    
    std::string path = getTempProjectPath();
    project.save(path);
    
    // Read raw JSON to verify version field exists
    std::ifstream file(path);
    std::string content((std::istreambuf_iterator<char>(file)),
                         std::istreambuf_iterator<char>());
    file.close();
    
    assert(content.find("\"version\"") != std::string::npos);
    assert(content.find("\"1\"") != std::string::npos || content.find(": 1") != std::string::npos);
    
    cleanupTempFile(path);
}

// ============================================================================
// Project file path tests
// ============================================================================

static void testProject_projectFilePath() {
    Project project;
    assert(project.filePath().empty());
    
    project.setName("Test");
    project.setRawFolder("/raw");
    project.setGameFolder("/game");
    
    std::string path = getTempProjectPath();
    project.save(path);
    
    assert(project.filePath() == path);
    
    cleanupTempFile(path);
}

static void testProject_loadedProjectHasFilePath() {
    Project original;
    original.setName("Path Test");
    original.setRawFolder("/raw");
    original.setGameFolder("/game");
    
    std::string path = getTempProjectPath();
    original.save(path);
    
    auto loaded = Project::load(path);
    assert(loaded.has_value());
    assert(loaded->filePath() == path);
    
    cleanupTempFile(path);
}

// ============================================================================
// Main test runner
// ============================================================================

int main() {
    // ClipState tests
    testClipState_defaultConstruction();
    testClipState_setNormalized();
    testClipState_setCompressed();
    testClipState_setTrimmed();
    testClipState_setExported();
    
    // ClipState status badge tests (for UI Status column)
    testClipState_statusBadge_noOperations();
    testClipState_statusBadge_trimmedOnly();
    testClipState_statusBadge_normalizedOnly();
    testClipState_statusBadge_compressedOnly();
    testClipState_statusBadge_exportedOnly();
    testClipState_statusBadge_multipleOperations();
    testClipState_statusBadge_allOperations();
    testClipState_statusBadge_commonWorkflow_normalizeAndExport();
    testClipState_statusBadge_commonWorkflow_fullProcessing();
    
    // ExportSettings tests
    testExportSettings_defaultConstruction();
    testExportSettings_customValues();
    
    // ProcessingSettings tests
    testProcessingSettings_defaultConstruction();
    
    // Project construction tests
    testProject_defaultConstruction();
    testProject_setName();
    testProject_setRawFolder();
    testProject_setGameFolder();
    testProject_setExportSettings();
    
    // Clip state management tests
    testProject_addClipState();
    testProject_updateClipState();
    testProject_updateClipState_withNormalization();
    testProject_updateClipState_withCompression();
    testProject_updateClipState_withTrim();
    testProject_updateClipState_multipleOperations();
    testProject_updateClipState_nonexistentClip();
    testProject_findClipState();
    testProject_removeClipState();
    testProject_clearClipStates();
    
    // Dirty state tests
    testProject_dirtyState_initiallyClean();
    testProject_dirtyState_afterModification();
    testProject_dirtyState_clearDirty();
    testProject_dirtyState_afterSave();
    
    // Serialization tests
    testProject_saveAndLoad_basicProperties();
    testProject_saveAndLoad_exportSettings();
    testProject_saveAndLoad_processingSettings();
    testProject_saveAndLoad_clipStates();
    testProject_load_nonexistentFile();
    testProject_load_invalidJson();
    testProject_load_missingRequiredFields();
    
    // Version tests
    testProject_versionField();
    
    // File path tests
    testProject_projectFilePath();
    testProject_loadedProjectHasFilePath();
    
    return 0;
}
