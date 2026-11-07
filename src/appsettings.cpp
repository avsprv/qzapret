#include "appsettings.h"

#include <QSettings>

namespace {
constexpr auto kThemeKey = "appearance/theme";
constexpr auto kLanguageKey = "appearance/language";
constexpr auto kAutoCheckKey = "update/autoCheck";
constexpr auto kCloseToTrayKey = "tray/closeToTray";
}

AppSettings::Theme AppSettings::theme()
{
    const auto value = QSettings().value(QLatin1String(kThemeKey), QStringLiteral("system")).toString();
    if (value == QLatin1String("light"))
        return Theme::Light;
    if (value == QLatin1String("dark"))
        return Theme::Dark;
    return Theme::System;
}

void AppSettings::setTheme(Theme value)
{
    const char *stored = "system";
    switch (value) {
    case Theme::Light: stored = "light"; break;
    case Theme::Dark: stored = "dark"; break;
    case Theme::System: break;
    }
    QSettings().setValue(QLatin1String(kThemeKey), QLatin1String(stored));
}

AppSettings::Language AppSettings::language()
{
    const auto value = QSettings().value(QLatin1String(kLanguageKey), QStringLiteral("system")).toString();
    if (value == QLatin1String("ru"))
        return Language::Russian;
    if (value == QLatin1String("en"))
        return Language::English;
    return Language::System;
}

void AppSettings::setLanguage(Language value)
{
    const char *stored = "system";
    switch (value) {
    case Language::Russian: stored = "ru"; break;
    case Language::English: stored = "en"; break;
    case Language::System: break;
    }
    QSettings().setValue(QLatin1String(kLanguageKey), QLatin1String(stored));
}

bool AppSettings::autoCheckUpdates()
{
    // Сохраняем прежний ключ и значение по умолчанию для обратной совместимости.
    return QSettings().value(QLatin1String(kAutoCheckKey), true).toBool();
}

void AppSettings::setAutoCheckUpdates(bool enabled)
{
    QSettings().setValue(QLatin1String(kAutoCheckKey), enabled);
}

bool AppSettings::closeToTray()
{
    // До появления настройки закрытие окна всегда прятало его в трей.
    return QSettings().value(QLatin1String(kCloseToTrayKey), true).toBool();
}

void AppSettings::setCloseToTray(bool enabled)
{
    QSettings().setValue(QLatin1String(kCloseToTrayKey), enabled);
}
