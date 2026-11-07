#include <cstdlib>

#include <QGuiApplication>
#include <QLocale>
#include <QQmlApplicationEngine>
#include <QQmlContext>
#include <QTranslator>

#include "applicationcontroller.h"
#include "configcontroller.h"
#include "i18n.h"
#include "profilecontroller.h"
#include "settingsmanager.h"

namespace {
QLocale localeForLanguage(const QString &language)
{
    if (language == QLatin1String("ru"))
        return QLocale(QLocale::Russian, QLocale::Russia);
    if (language == QLatin1String("en"))
        return QLocale(QLocale::English, QLocale::UnitedStates);
    return QLocale();
}
} // namespace

int main(int argc, char *argv[])
{
    QGuiApplication app(argc, argv);
    QCoreApplication::setApplicationName(QStringLiteral("zapret2applet"));
    QCoreApplication::setApplicationVersion(QStringLiteral(QZAPRET_VERSION));
    QGuiApplication::setDesktopFileName(QStringLiteral("zapret2applet"));

    ApplicationController controller;
    SettingsManager settingsManager;
    ConfigController configController;
    ProfileController profileController;

    const QLocale initialLocale = localeForLanguage(settingsManager.language());
    QLocale::setDefault(initialLocale);
    QList<QTranslator *> translators = I18n::install(app, initialLocale);
    QQmlApplicationEngine engine;
    engine.rootContext()->setContextProperty(QStringLiteral("appController"), &controller);
    engine.rootContext()->setContextProperty(QStringLiteral("settingsManager"), &settingsManager);
    engine.rootContext()->setContextProperty(QStringLiteral("configController"), &configController);
    engine.rootContext()->setContextProperty(QStringLiteral("profileController"), &profileController);
    engine.loadFromModule(QStringLiteral("org.avsprv.zapret2applet"), QStringLiteral("Main"));

    if (engine.rootObjects().isEmpty())
        return EXIT_FAILURE;

    QObject::connect(&settingsManager, &SettingsManager::languageChanged, &app, [&]() {
        for (QTranslator *translator : translators)
            delete translator;
        const QLocale locale = localeForLanguage(settingsManager.language());
        QLocale::setDefault(locale);
        translators = I18n::install(app, locale);
        engine.retranslate();
    });

    return app.exec();
}
