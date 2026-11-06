#pragma once

#include <QString>

namespace ZapretPaths {

// Куда установлен zapret2 (по умолчанию — как в документации апстрима).
// QZAPRET_BASE — только для отладки GUI; хелпер этот путь не использует.
inline QString baseDir()
{
    const QString env = qEnvironmentVariable("QZAPRET_BASE");
    return env.isEmpty() ? QStringLiteral("/opt/zapret2") : env;
}

} // namespace ZapretPaths
