/**
 * @file ProjectManager.h
 * @brief Manages project lifecycle and recent projects list.
 *
 * Provides new/open/save/save-as operations, tracks dirty state,
 * and maintains a list of recently opened projects.
 */

#pragma once

#include <QObject>
#include <QSettings>
#include <QStringList>
#include <memory>
#include "Project.h"

/**
 * @class ProjectManager
 * @brief Manages the current project and recent projects history.
 *
 * This is a QObject to enable signal/slot connections with the UI.
 * Owns the current Project instance and handles persistence.
 */
class ProjectManager final : public QObject {
    Q_OBJECT

public:
    static constexpr int kMaxRecentProjects = 10;
    
    explicit ProjectManager(QObject* parent = nullptr);
    ~ProjectManager() override = default;
    
    // --- Project access ---
    
    /**
     * @brief Get the current project.
     * @return Reference to the current project.
     */
    [[nodiscard]] Project& project() noexcept { return project_; }
    [[nodiscard]] const Project& project() const noexcept { return project_; }
    
    /**
     * @brief Check if a project is currently open.
     * @return True if a project exists (even if unsaved).
     */
    [[nodiscard]] bool hasProject() const noexcept { return hasProject_; }
    
    /**
     * @brief Check if current project has unsaved changes.
     */
    [[nodiscard]] bool isDirty() const noexcept { return project_.isDirty(); }
    
    /**
     * @brief Get the display name for the window title.
     * @return Project name or "Untitled" if no name set.
     */
    [[nodiscard]] QString displayName() const;
    
    // --- Project operations ---
    
    /**
     * @brief Create a new empty project.
     * 
     * If there are unsaved changes, the caller should prompt to save first.
     */
    void newProject();
    
    /**
     * @brief Create a new project with initial settings.
     * @param name Project name
     * @param rawFolder Path to RAW audio folder
     * @param gameFolder Path to game audio output folder
     */
    void newProject(const QString& name, const QString& rawFolder, const QString& gameFolder);
    
    /**
     * @brief Open a project from file.
     * @param path Path to .wooshp file
     * @return True if project was loaded successfully
     */
    [[nodiscard]] bool openProject(const QString& path);
    
    /**
     * @brief Save current project.
     * 
     * If project has no file path, this will fail.
     * Use saveProjectAs() for new projects.
     * @return True if save succeeded
     */
    [[nodiscard]] bool saveProject();
    
    /**
     * @brief Save current project to a new path.
     * @param path Path for the .wooshp file
     * @return True if save succeeded
     */
    [[nodiscard]] bool saveProjectAs(const QString& path);
    
    /**
     * @brief Close the current project.
     * 
     * Caller should check isDirty() and prompt to save first.
     */
    void closeProject();
    
    // --- Recent projects ---
    
    /**
     * @brief Get list of recently opened project paths.
     */
    [[nodiscard]] QStringList recentProjects() const;
    
    /**
     * @brief Clear the recent projects list.
     */
    void clearRecentProjects();
    
    /**
     * @brief Remove a path from recent projects (e.g., if file no longer exists).
     */
    void removeFromRecentProjects(const QString& path);

Q_SIGNALS:
    /**
     * @brief Emitted when a project is opened, created, or closed.
     */
    void projectChanged();
    
    /**
     * @brief Emitted when project is saved.
     */
    void projectSaved();
    
    /**
     * @brief Emitted when dirty state changes.
     * @param dirty True if project has unsaved changes.
     */
    void dirtyStateChanged(bool dirty);
    
    /**
     * @brief Emitted when recent projects list changes.
     */
    void recentProjectsChanged();

private:
    void addToRecentProjects(const QString& path);
    void loadRecentProjects();
    void saveRecentProjects();
    void checkDirtyState();
    
    Project project_;
    bool hasProject_{false};
    bool lastDirtyState_{false};
    
    QStringList recentProjects_;
    std::unique_ptr<QSettings> settings_;
    
    static constexpr const char* kSettingsOrg = "Woosh";
    static constexpr const char* kSettingsApp = "WooshEditor";
    static constexpr const char* kRecentProjectsKey = "RecentProjects";
};
