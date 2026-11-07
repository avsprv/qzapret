#include "appsettings.h"

#include <QApplication>
#include <QColor>
#include <QPalette>
#include <QSettings>
#include <QStyle>

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

void AppSettings::applyTheme(QApplication &app)
{
    const Theme selected = theme();
    if (selected == Theme::System) {
        app.setPalette(app.style()->standardPalette());
        return;
    }

    QPalette palette = app.style()->standardPalette();
    if (selected == Theme::Light) {
        palette.setColor(QPalette::Window, QColor(250, 250, 250));
        palette.setColor(QPalette::WindowText, QColor(35, 35, 35));
        palette.setColor(QPalette::Base, QColor(255, 255, 255));
        palette.setColor(QPalette::AlternateBase, QColor(245, 245, 245));
        palette.setColor(QPalette::ToolTipBase, QColor(255, 255, 220));
        palette.setColor(QPalette::ToolTipText, QColor(25, 25, 25));
        palette.setColor(QPalette::Text, QColor(35, 35, 35));
        palette.setColor(QPalette::Button, QColor(242, 242, 242));
        palette.setColor(QPalette::ButtonText, QColor(35, 35, 35));
        palette.setColor(QPalette::BrightText, QColor(190, 0, 0));
        palette.setColor(QPalette::Highlight, QColor(48, 125, 190));
        palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
    } else {
        palette.setColor(QPalette::Window, QColor(35, 38, 41));
        palette.setColor(QPalette::WindowText, QColor(235, 235, 235));
        palette.setColor(QPalette::Base, QColor(27, 30, 33));
        palette.setColor(QPalette::AlternateBase, QColor(43, 46, 49));
        palette.setColor(QPalette::ToolTipBase, QColor(45, 48, 51));
        palette.setColor(QPalette::ToolTipText, QColor(245, 245, 245));
        palette.setColor(QPalette::Text, QColor(235, 235, 235));
        palette.setColor(QPalette::Button, QColor(45, 48, 51));
        palette.setColor(QPalette::ButtonText, QColor(235, 235, 235));
        palette.setColor(QPalette::BrightText, QColor(255, 85, 85));
        palette.setColor(QPalette::Highlight, QColor(55, 125, 185));
        palette.setColor(QPalette::HighlightedText, QColor(255, 255, 255));
        palette.setColor(QPalette::Disabled, QPalette::WindowText, QColor(125, 125, 125));
        palette.setColor(QPalette::Disabled, QPalette::Text, QColor(125, 125, 125));
        palette.setColor(QPalette::Disabled, QPalette::ButtonText, QColor(125, 125, 125));
    }
    app.setPalette(palette);
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
