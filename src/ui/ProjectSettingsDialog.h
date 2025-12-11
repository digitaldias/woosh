/**
 * @file ProjectSettingsDialog.h
 * @brief Dialog for editing Woosh project settings.
 */

#pragma once

#include <QDialog>
#include "core/Project.h"

class QLineEdit;
class QComboBox;
class QSpinBox;
class QPushButton;
class QTabWidget;

/**
 * @brief Dialog for editing project settings.
 * 
 * Allows editing:
 * - Project name and folders
 * - Export format (WAV, MP3, OGG)
 * - Bitrate for lossy formats
 * - Metadata (artist, album/game name, comment)
 */
class ProjectSettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit ProjectSettingsDialog(QWidget* parent = nullptr);

    /** @brief Load settings from a project. */
    void loadFromProject(const Project& project);

    /** @brief Apply settings to a project. */
    void applyToProject(Project& project) const;

    // Individual getters for settings
    [[nodiscard]] QString projectName() const;
    [[nodiscard]] QString rawFolder() const;
    [[nodiscard]] QString gameFolder() const;
    [[nodiscard]] ExportFormat exportFormat() const;
    [[nodiscard]] int bitrate() const;
    [[nodiscard]] QString metadataArtist() const;
    [[nodiscard]] QString metadataAlbum() const;
    [[nodiscard]] QString metadataComment() const;

private Q_SLOTS:
    void browseRawFolder();
    void browseGameFolder();
    void onFormatChanged(int index);
    void validateInputs();

private:
    void setupUi();
    void setupGeneralTab(QTabWidget* tabs);
    void setupExportTab(QTabWidget* tabs);
    void setupMetadataTab(QTabWidget* tabs);

    // General tab
    QLineEdit* nameEdit_{nullptr};
    QLineEdit* rawFolderEdit_{nullptr};
    QLineEdit* gameFolderEdit_{nullptr};
    QPushButton* browseRawBtn_{nullptr};
    QPushButton* browseGameBtn_{nullptr};

    // Export tab
    QComboBox* formatCombo_{nullptr};
    QComboBox* bitrateCombo_{nullptr};

    // Metadata tab
    QLineEdit* artistEdit_{nullptr};
    QLineEdit* albumEdit_{nullptr};
    QLineEdit* commentEdit_{nullptr};

    // Buttons
    QPushButton* okBtn_{nullptr};
    QPushButton* cancelBtn_{nullptr};
};
