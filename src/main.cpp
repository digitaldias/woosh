/**
 * @file main.cpp
 * @brief Woosh Audio Batch Editor entry point.
 *
 * Sets up the Qt application with dark theme styling and launches MainWindow.
 */

#include <QApplication>
#include <QIcon>
#include <QFont>
#include <QStyleFactory>
#include "ui/MainWindow.h"

/**
 * @brief Apply a modern dark theme stylesheet to the application.
 */
static void applyDarkTheme(QApplication& app) {
    // Use Fusion style as base (works well with custom palettes)
    app.setStyle(QStyleFactory::create("Fusion"));

    // Dark color palette
    QPalette darkPalette;
    darkPalette.setColor(QPalette::Window, QColor(45, 48, 55));
    darkPalette.setColor(QPalette::WindowText, QColor(220, 220, 225));
    darkPalette.setColor(QPalette::Base, QColor(30, 33, 38));
    darkPalette.setColor(QPalette::AlternateBase, QColor(38, 41, 48));
    darkPalette.setColor(QPalette::ToolTipBase, QColor(50, 53, 60));
    darkPalette.setColor(QPalette::ToolTipText, QColor(220, 220, 225));
    darkPalette.setColor(QPalette::Text, QColor(220, 220, 225));
    darkPalette.setColor(QPalette::Button, QColor(50, 54, 62));
    darkPalette.setColor(QPalette::ButtonText, QColor(220, 220, 225));
    darkPalette.setColor(QPalette::BrightText, Qt::white);
    darkPalette.setColor(QPalette::Link, QColor(80, 180, 220));
    darkPalette.setColor(QPalette::Highlight, QColor(65, 130, 180));
    darkPalette.setColor(QPalette::HighlightedText, Qt::white);

    // Disabled colors
    darkPalette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(120, 120, 125));
    darkPalette.setColor(QPalette::Disabled, QPalette::Text, QColor(120, 120, 125));
    darkPalette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(120, 120, 125));

    app.setPalette(darkPalette);

    // Global stylesheet for fine-tuned appearance
    app.setStyleSheet(R"(
        /* Main window and widgets */
        QMainWindow, QDialog {
            background-color: #2d3037;
        }

        /* Menu bar */
        QMenuBar {
            background-color: #282b30;
            border-bottom: 1px solid #3a3e47;
            padding: 2px;
        }
        QMenuBar::item {
            padding: 5px 10px;
            background: transparent;
            border-radius: 4px;
        }
        QMenuBar::item:selected {
            background-color: #3d424d;
        }
        QMenu {
            background-color: #2d3037;
            border: 1px solid #3a3e47;
            padding: 4px;
        }
        QMenu::item {
            padding: 6px 25px 6px 20px;
            border-radius: 3px;
        }
        QMenu::item:selected {
            background-color: #4180b4;
        }
        QMenu::separator {
            height: 1px;
            background: #3a3e47;
            margin: 4px 8px;
        }

        /* Group boxes */
        QGroupBox {
            font-weight: bold;
            border: 1px solid #3a3e47;
            border-radius: 6px;
            margin-top: 8px;
            padding-top: 8px;
            background-color: #32363e;
        }
        QGroupBox::title {
            subcontrol-origin: margin;
            left: 10px;
            padding: 0 6px;
            color: #9cc4e8;
        }

        /* Buttons - Default style */
        QPushButton {
            background-color: #3d424d;
            border: 1px solid #4a505c;
            border-radius: 6px;
            padding: 8px 16px;
            min-height: 24px;
            min-width: 32px;
            font-size: 13px;
            font-weight: 500;
            color: #dcdce1;
        }
        QPushButton:hover {
            background-color: #4a5262;
            border-color: #6080a0;
        }
        QPushButton:pressed {
            background-color: #4180b4;
            border-color: #5090c4;
        }
        QPushButton:disabled {
            background-color: #2d3037;
            border-color: #3a3e47;
            color: #606068;
        }
        
        /* Transport buttons (play, stop, zoom) - larger and more prominent */
        QPushButton[class="transport"] {
            min-width: 40px;
            min-height: 36px;
            font-size: 16px;
            padding: 6px 12px;
            border-radius: 8px;
        }
        
        /* Primary action buttons (Export, Apply, etc.) */
        QPushButton[class="primary"] {
            background-color: #3070a0;
            border-color: #4080b0;
            font-weight: bold;
        }
        QPushButton[class="primary"]:hover {
            background-color: #4088b8;
            border-color: #50a0d0;
        }
        QPushButton[class="primary"]:pressed {
            background-color: #3090c0;
        }
        QPushButton[class="primary"]:disabled {
            background-color: #2a3540;
            border-color: #3a4550;
            color: #606068;
        }
        
        /* Secondary action buttons (Normalize, Compress) */
        QPushButton[class="secondary"] {
            background-color: #404854;
            border-color: #505a68;
        }
        QPushButton[class="secondary"]:hover {
            background-color: #4a5464;
            border-color: #6080a0;
        }

        /* Line edits */
        QLineEdit {
            background-color: #252830;
            border: 1px solid #3a3e47;
            border-radius: 4px;
            padding: 4px 8px;
            selection-background-color: #4180b4;
        }
        QLineEdit:focus {
            border-color: #4180b4;
        }
        QLineEdit:read-only {
            background-color: #2a2d34;
        }

        /* Check boxes */
        QCheckBox {
            spacing: 8px;
        }
        QCheckBox::indicator {
            width: 16px;
            height: 16px;
            border-radius: 3px;
            border: 1px solid #4a505c;
            background-color: #252830;
        }
        QCheckBox::indicator:checked {
            background-color: #4180b4;
            border-color: #5090c4;
        }
        QCheckBox::indicator:hover {
            border-color: #5a6070;
        }

        /* Table view */
        QTableView {
            background-color: #1e2126;
            alternate-background-color: #252830;
            gridline-color: #32363e;
            border: 1px solid #3a3e47;
            border-radius: 4px;
            selection-background-color: #3d6a8f;
        }
        QTableView::item {
            padding: 4px;
        }
        QTableView::item:selected {
            background-color: #3d6a8f;
        }
        QHeaderView::section {
            background-color: #32363e;
            border: none;
            border-right: 1px solid #3a3e47;
            border-bottom: 1px solid #3a3e47;
            padding: 6px 8px;
            font-weight: bold;
            color: #a0a0a8;
        }
        QHeaderView::section:hover {
            background-color: #3a3e47;
            color: #dcdce1;
        }

        /* Scroll bars */
        QScrollBar:vertical {
            background-color: #252830;
            width: 12px;
            border-radius: 6px;
            margin: 2px;
        }
        QScrollBar::handle:vertical {
            background-color: #4a505c;
            border-radius: 5px;
            min-height: 30px;
            margin: 1px;
        }
        QScrollBar::handle:vertical:hover {
            background-color: #5a6070;
        }
        QScrollBar::add-line:vertical, QScrollBar::sub-line:vertical {
            height: 0px;
        }
        QScrollBar:horizontal {
            background-color: #252830;
            height: 12px;
            border-radius: 6px;
            margin: 2px;
        }
        QScrollBar::handle:horizontal {
            background-color: #4a505c;
            border-radius: 5px;
            min-width: 30px;
            margin: 1px;
        }
        QScrollBar::handle:horizontal:hover {
            background-color: #5a6070;
        }
        QScrollBar::add-line:horizontal, QScrollBar::sub-line:horizontal {
            width: 0px;
        }

        /* Progress bar */
        QProgressBar {
            background-color: #252830;
            border: 1px solid #3a3e47;
            border-radius: 4px;
            text-align: center;
            color: #dcdce1;
        }
        QProgressBar::chunk {
            background-color: #4180b4;
            border-radius: 3px;
        }

        /* Status bar */
        QStatusBar {
            background-color: #282b30;
            border-top: 1px solid #3a3e47;
        }

        /* Splitter */
        QSplitter::handle {
            background-color: #3a3e47;
        }
        QSplitter::handle:horizontal {
            width: 3px;
        }
        QSplitter::handle:vertical {
            height: 3px;
        }
        QSplitter::handle:hover {
            background-color: #4180b4;
        }

        /* Tool tips */
        QToolTip {
            background-color: #3a3e47;
            border: 1px solid #4a505c;
            border-radius: 4px;
            padding: 4px 8px;
            color: #dcdce1;
        }

        /* Labels */
        QLabel {
            color: #c0c0c8;
        }

        /* Frame separators */
        QFrame[frameShape="5"] {
            color: #3a3e47;
        }
    )");
}

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);

    // Set application metadata
    QCoreApplication::setOrganizationName("Woosh");
    QCoreApplication::setApplicationName("Woosh Audio Editor");
    QCoreApplication::setApplicationVersion("0.1.0");

    // Apply dark theme
    applyDarkTheme(app);

    // Set application icon
    QIcon appIcon(":/images/icon.png");
    app.setWindowIcon(appIcon);

    // Create and show main window
    MainWindow window;
    window.setWindowIcon(appIcon);
    window.show();

    return app.exec();
}
