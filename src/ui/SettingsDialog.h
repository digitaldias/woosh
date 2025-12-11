/**
 * @file SettingsDialog.h
 * @brief Settings dialog for Woosh preferences.
 */

#pragma once

#include <QDialog>

class QCheckBox;
class QPushButton;

/**
 * @class SettingsDialog
 * @brief Modal dialog for application settings.
 *
 * Settings include:
 *  - Show/hide column tooltips
 *  - Clear recent folders/files history
 */
class SettingsDialog final : public QDialog {
    Q_OBJECT

public:
    explicit SettingsDialog(QWidget* parent = nullptr);

    [[nodiscard]] bool showColumnTooltips() const;
    void setShowColumnTooltips(bool show);

Q_SIGNALS:
    /** @brief Emitted when user clicks Clear History. */
    void clearHistoryRequested();

private:
    void setupUi();
    void onClearHistoryClicked();

    QCheckBox* tooltipsCheck_ = nullptr;
    QPushButton* clearHistoryBtn_ = nullptr;
};

