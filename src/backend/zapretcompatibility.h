#pragma once

#include <QString>
#include <QStringList>

namespace ZapretCompatibility {

struct Report {
    bool compatible = false;
    QString version;
    QStringList missingComponents;
};

Report inspect(const QString &baseDir);

} // namespace ZapretCompatibility
