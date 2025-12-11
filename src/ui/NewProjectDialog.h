/**
 * @file NewProjectDialog.h
 * @brief Dialog for creating a new Woosh project.
 */

#pragma once

#include <QDialog>
#include <QString>

class QLineEdit;
class QPushButton;

/**
 * @brief Dialog for creating a new Woosh project.
 * 
 * Allows user to specify:
 * - Project name
 * - Game name (for metadata)
 * - Author name (for metadata)
 * - RAW audio folder (source files)
 * - Game audio folder (processed output)
 */
class NewProjectDialog final : public QDialog {
    Q_OBJECT

public:
    explicit NewProjectDialog(QWidget* parent = nullptr);

    /** @brief Get the entered project name. */
    [[nodiscard]] QString projectName() const;

    /** @brief Get the game name for metadata. */
    [[nodiscard]] QString gameName() const;

    /** @brief Get the author name for metadata. */
    [[nodiscard]] QString authorName() const;

    /** @brief Get the selected RAW audio folder path. */
    [[nodiscard]] QString rawFolder() const;

    /** @brief Get the selected Game audio folder path. */
    [[nodiscard]] QString gameFolder() const;

    /** @brief Set default author name (from settings). */
    void setDefaultAuthorName(const QString& name);

private Q_SLOTS:
    void browseRawFolder();
    void browseGameFolder();
    void validateInputs();

private:
    void setupUi();

    QLineEdit* nameEdit_{nullptr};
    QLineEdit* gameNameEdit_{nullptr};
    QLineEdit* authorNameEdit_{nullptr};
    QLineEdit* rawFolderEdit_{nullptr};
    QLineEdit* gameFolderEdit_{nullptr};
    QPushButton* browseRawBtn_{nullptr};
    QPushButton* browseGameBtn_{nullptr};
    QPushButton* createBtn_{nullptr};
    QPushButton* cancelBtn_{nullptr};
};
