#include <QApplication>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qzapret"));
    QApplication::setApplicationVersion(QStringLiteral(QZAPRET_VERSION));
    QApplication::setDesktopFileName(QStringLiteral("qzapret"));

    MainWindow window;
    window.show();
    return app.exec();
}
