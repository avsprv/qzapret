#include <QApplication>
#include <QCommandLineParser>

#include "mainwindow.h"

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("qzapret"));
    QApplication::setApplicationVersion(QStringLiteral(QZAPRET_VERSION));
    QApplication::setDesktopFileName(QStringLiteral("qzapret"));
    // Окно прячется в трей, поэтому закрытие последнего окна не должно завершать приложение.
    QApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("Unofficial Qt6 GUI for zapret2"));
    parser.addHelpOption();
    parser.addVersionOption();
    QCommandLineOption minimized(QStringLiteral("minimized"),
                                 QStringLiteral("Start hidden in the system tray"));
    parser.addOption(minimized);
    parser.process(app);

    MainWindow window;
    // Без трея скрытый запуск бессмысленен — окно всё равно покажем.
    if (!(parser.isSet(minimized) && window.trayAvailable()))
        window.show();
    // Если трея нет, закрытие окна должно завершать приложение.
    if (!window.trayAvailable())
        QApplication::setQuitOnLastWindowClosed(true);
    return app.exec();
}
