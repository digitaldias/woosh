/**
 * @file ProjectManager.cpp
 * @brief Implementation of ProjectManager.
 */

#include "ProjectManager.h"
#include <QFileInfo>

ProjectManager::ProjectManager(QObject* parent)
    : QObject(parent)
    , settings_(std::make_unique<QSettings>(kSettingsOrg, kSettingsApp))
{
    loadRecentProjects();
}

QString ProjectManager::displayName() const {
    if (!hasProject_) {
        return QString();
    }
    
    if (project_.name().empty()) {
        return QStringLiteral("Untitled");
    }
    
    return QString::fromStdString(project_.name());
}

void ProjectManager::newProject() {
    project_ = Project();
    hasProject_ = true;
    lastDirtyState_ = false;
    
    Q_EMIT projectChanged();
    Q_EMIT dirtyStateChanged(false);
}

void ProjectManager::newProject(const QString& name, const QString& rawFolder, const QString& gameFolder) {
    project_ = Project();
    project_.setName(name.toStdString());
    project_.setRawFolder(rawFolder.toStdString());
    project_.setGameFolder(gameFolder.toStdString());
    
    hasProject_ = true;
    lastDirtyState_ = true; // New project with data is dirty until saved
    
    Q_EMIT projectChanged();
    Q_EMIT dirtyStateChanged(true);
}

bool ProjectManager::openProject(const QString& path) {
    auto loaded = Project::load(path.toStdString());
    if (!loaded) {
        removeFromRecentProjects(path);
        return false;
    }
    
    project_ = std::move(*loaded);
    hasProject_ = true;
    lastDirtyState_ = false;
    
    addToRecentProjects(path);
    
    Q_EMIT projectChanged();
    Q_EMIT dirtyStateChanged(false);
    
    return true;
}

bool ProjectManager::saveProject() {
    if (project_.filePath().empty()) {
        return false;
    }
    
    bool success = project_.save(project_.filePath());
    if (success) {
        lastDirtyState_ = false;
        Q_EMIT projectSaved();
        Q_EMIT dirtyStateChanged(false);
    }
    
    return success;
}

bool ProjectManager::saveProjectAs(const QString& path) {
    bool success = project_.save(path.toStdString());
    if (success) {
        lastDirtyState_ = false;
        addToRecentProjects(path);
        Q_EMIT projectSaved();
        Q_EMIT dirtyStateChanged(false);
    }
    
    return success;
}

void ProjectManager::closeProject() {
    project_ = Project();
    hasProject_ = false;
    lastDirtyState_ = false;
    
    Q_EMIT projectChanged();
    Q_EMIT dirtyStateChanged(false);
}

QStringList ProjectManager::recentProjects() const {
    return recentProjects_;
}

void ProjectManager::clearRecentProjects() {
    recentProjects_.clear();
    saveRecentProjects();
    Q_EMIT recentProjectsChanged();
}

void ProjectManager::removeFromRecentProjects(const QString& path) {
    if (recentProjects_.removeAll(path) > 0) {
        saveRecentProjects();
        Q_EMIT recentProjectsChanged();
    }
}

void ProjectManager::addToRecentProjects(const QString& path) {
    // Remove if already exists (will be re-added at front)
    recentProjects_.removeAll(path);
    
    // Add to front
    recentProjects_.prepend(path);
    
    // Trim to max size
    while (recentProjects_.size() > kMaxRecentProjects) {
        recentProjects_.removeLast();
    }
    
    saveRecentProjects();
    Q_EMIT recentProjectsChanged();
}

void ProjectManager::loadRecentProjects() {
    recentProjects_ = settings_->value(kRecentProjectsKey).toStringList();
    
    // Remove entries that no longer exist
    QStringList valid;
    for (const QString& path : recentProjects_) {
        if (QFileInfo::exists(path)) {
            valid.append(path);
        }
    }
    
    if (valid.size() != recentProjects_.size()) {
        recentProjects_ = valid;
        saveRecentProjects();
    }
}

void ProjectManager::saveRecentProjects() {
    settings_->setValue(kRecentProjectsKey, recentProjects_);
    settings_->sync();
}

void ProjectManager::checkDirtyState() {
    bool currentDirty = project_.isDirty();
    if (currentDirty != lastDirtyState_) {
        lastDirtyState_ = currentDirty;
        Q_EMIT dirtyStateChanged(currentDirty);
    }
}
