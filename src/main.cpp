#include <QApplication>
#include <QIcon>
#include "ui/MainWindow.h"

int main(int argc, char *argv[]) {
    QApplication app(argc, argv);
    app.setWindowIcon(QIcon(":/images/icon.png"));
    MainWindow window;
    window.setWindowIcon(QIcon(":/images/icon.png"));
    window.show();
    return app.exec();
}



