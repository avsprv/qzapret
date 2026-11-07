#include "settingsmanager.h"

#include "appsettings.h"

SettingsManager::SettingsManager(QObject *parent)
    : QObject(parent)
{
}

QString SettingsManager::theme() const
{
    switch (AppSettings::theme()) {
    case AppSettings::Theme::Light: return QStringLiteral("light");
    case AppSettings::Theme::Dark: return QStringLiteral("dark");
    case AppSettings::Theme::System:
    default: return QStringLiteral("system");
    }
}

void SettingsManager::setTheme(const QString &value)
{
    AppSettings::Theme selected = AppSettings::Theme::System;
    if (value == QLatin1String("light"))
        selected = AppSettings::Theme::Light;
    else if (value == QLatin1String("dark"))
        selected = AppSettings::Theme::Dark;

    if (AppSettings::theme() == selected)
        return;
    AppSettings::setTheme(selected);
    emit themeChanged();
}

QString SettingsManager::language() const
{
    switch (AppSettings::language()) {
    case AppSettings::Language::Russian: return QStringLiteral("ru");
    case AppSettings::Language::English: return QStringLiteral("en");
    case AppSettings::Language::System:
    default: return QStringLiteral("system");
    }
}

void SettingsManager::setLanguage(const QString &value)
{
    AppSettings::Language selected = AppSettings::Language::System;
    if (value == QLatin1String("ru"))
        selected = AppSettings::Language::Russian;
    else if (value == QLatin1String("en"))
        selected = AppSettings::Language::English;

    if (AppSettings::language() == selected)
        return;
    AppSettings::setLanguage(selected);
    emit languageChanged();
}

bool SettingsManager::autoCheckUpdates() const
{
    return AppSettings::autoCheckUpdates();
}

void SettingsManager::setAutoCheckUpdates(bool enabled)
{
    if (AppSettings::autoCheckUpdates() == enabled)
        return;
    AppSettings::setAutoCheckUpdates(enabled);
    emit autoCheckUpdatesChanged();
}

bool SettingsManager::closeToTray() const
{
    return AppSettings::closeToTray();
}

void SettingsManager::setCloseToTray(bool enabled)
{
    if (AppSettings::closeToTray() == enabled)
        return;
    AppSettings::setCloseToTray(enabled);
    emit closeToTrayChanged();
}
