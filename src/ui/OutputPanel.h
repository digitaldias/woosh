/**
 * @file OutputPanel.h
 * @brief Panel widget for output folder selection and export controls.
 *
 * Provides an output folder picker, an option to overwrite original files,
 * and buttons to export selected or all clips.
 */

#pragma once

#include <QGroupBox>
#include <QString>

class QLineEdit;
class QPushButton;
class QCheckBox;

/**
 * @class OutputPanel
 * @brief A grouped panel for output/export controls.
 *
 * Emits signals when the user clicks "Export Selected" or "Export All".
 * The parent (MainWindow) handles the actual export logic.
 */
class OutputPanel : public QGroupBox {
    Q_OBJECT

public:
    /**
     * @brief Construct the output panel.
     * @param parent Optional parent widget.
     */
    explicit OutputPanel(QWidget* parent = nullptr);

    /**
     * @brief Get the currently selected output folder path.
     * @return The output folder path, or empty string if not set.
     */
    QString outputFolder() const;

    /**
     * @brief Set the output folder path.
     * @param path The folder path to display.
     */
    void setOutputFolder(const QString& path);

    /**
     * @brief Check if "overwrite originals" is enabled.
     * @return True if overwrite is checked, false otherwise.
     */
    bool overwriteOriginals() const;

Q_SIGNALS:
    /**
     * @brief Emitted when user clicks "Export Selected".
     */
    void exportSelected();

    /**
     * @brief Emitted when user clicks "Export All".
     */
    void exportAll();

    /**
     * @brief Emitted when the output folder changes.
     * @param newPath The new output folder path.
     */
    void outputFolderChanged(const QString& newPath);

private Q_SLOTS:
    void onBrowseClicked();

private:
    void setupUi();

    QLineEdit* folderEdit_ = nullptr;
    QPushButton* browseBtn_ = nullptr;
    QCheckBox* overwriteCheck_ = nullptr;
    QPushButton* exportSelectedBtn_ = nullptr;
    QPushButton* exportAllBtn_ = nullptr;
};

