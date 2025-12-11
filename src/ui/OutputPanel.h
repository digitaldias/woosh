/**
 * @file OutputPanel.h
 * @brief Panel widget for output folder display and export controls.
 *
 * Displays the output folder (configured via project), an option to overwrite 
 * original files, and buttons to export selected or all clips.
 */

#pragma once

#include <QGroupBox>
#include <QString>

class QLabel;
class QPushButton;
class QCheckBox;

/**
 * @class OutputPanel
 * @brief A grouped panel for output/export controls.
 *
 * Emits signals when the user clicks "Export Selected" or "Export All".
 * The parent (MainWindow) handles the actual export logic.
 * The output folder is displayed as read-only (set from project settings).
 */
class OutputPanel final : public QGroupBox {
    Q_OBJECT

public:
    /**
     * @brief Construct the output panel.
     * @param parent Optional parent widget.
     */
    explicit OutputPanel(QWidget* parent = nullptr);

    /**
     * @brief Get the currently displayed output folder path.
     * @return The output folder path, or empty string if not set.
     */
    [[nodiscard]] QString outputFolder() const;

    /**
     * @brief Set the output folder path (read-only display).
     * @param path The folder path to display.
     */
    void setOutputFolder(const QString& path);

    /**
     * @brief Check if "overwrite originals" is enabled.
     * @return True if overwrite is checked, false otherwise.
     */
    [[nodiscard]] bool overwriteOriginals() const;

Q_SIGNALS:
    /**
     * @brief Emitted when user clicks "Export Selected".
     */
    void exportSelected();

    /**
     * @brief Emitted when user clicks "Export All".
     */
    void exportAll();

private:
    void setupUi();

    QLabel* folderLabel_ = nullptr;
    QString outputFolder_;
    QCheckBox* overwriteCheck_ = nullptr;
    QPushButton* exportSelectedBtn_ = nullptr;
    QPushButton* exportAllBtn_ = nullptr;
};

