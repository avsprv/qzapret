#pragma once

#include <QString>

namespace AppSettings {

// Стабильные значения хранятся в QSettings. Не менять organization/application name
// без отдельной миграции существующих пользовательских настроек.
enum class Theme {
    System,
    Light,
    Dark
};

enum class Language {
    System,
    Russian,
    English
};

Theme theme();
void setTheme(Theme value);

Language language();
void setLanguage(Language value);

bool autoCheckUpdates();
void setAutoCheckUpdates(bool enabled);

bool closeToTray();
void setCloseToTray(bool enabled);

} // namespace AppSettings
