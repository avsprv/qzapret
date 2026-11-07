#include <cstdlib>

#include <QGuiApplication>
#include <QQmlApplicationEngine>
#include <QQmlContext>

#include "applicationcontroller.h"
#include "configcontroller.h"
#include "settingsmanager.h"

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("zapret2applet"));
    QCoreApplication::setApplicationVersion(QStringLiteral(QZAPRET_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("zapret2applet"));

    ApplicationController controller;
    SettingsManager settingsManager;
    ConfigController configController;
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsManager"), &settingsManager);\n    engine.rootContext()->setContextProperty(QStringLiteral("configController"), &configController);
    engine.loadFromModule(QStringLiteral("org.avsprv.zapret2applet"), QStringLiteral("Main"));

    if (engine.rootObjects().isEmpty())
        return EXIT_FAILURE;

    return app.exec();
}
