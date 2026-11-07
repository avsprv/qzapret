#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "applicationcontroller.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setOrganizationName(QStringLiteral("avsprv"));
    QCoreApplication::setApplicationName(QStringLiteral("zapret2applet"));
    QCoreApplication::setApplicationVersion(QStringLiteral(QZAPRET_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("zapret2applet"));

    ApplicationController controller;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    engine.loadFromModule(QStringLiteral("org.avsprv.zapret2applet"), QStringLiteral("Main"));

    if (engine.rootObjects().isEmpty())
        return EXIT_FAILURE;

    return app.exec();
}
