#include "zapretcompatibility.h"

#include <QDir>
#include <QFile>
#include <QFileInfo>
#include <QRegularExpression>

namespace ZapretCompatibility {
namespace {

bool containsAll(const QString &text, const QStringList &markers, QStringList *missing)
{
    bool ok = true;
    for (const QString &marker : markers) {
        if (!text.contains(marker)) {
            missing->append(QStringLiteral("blockcheck2.sh: %1").arg(marker));
            ok = false;
        }
    }
    return ok;
}

} // namespace

Report inspect(const QString &baseDir)
{
    Report report;
    const QDir base(baseDir);
    const QStringList requiredFiles{
        QStringLiteral("blockcheck2.sh"),
        QStringLiteral("config.default"),
        QStringLiteral("common/base.sh"),
        QStringLiteral("common/dialog.sh"),
        QStringLiteral("blockcheck2.d/standard/10-http-basic.sh"),
        QStringLiteral("nfq2/nfqws2"),
        QStringLiteral("docs/changes.txt")
    };

    for (const QString &path : requiredFiles) {
        const QFileInfo info(base.filePath(path));
        if (!info.exists() || !info.isFile())
            report.missingComponents.append(path);
    }

    QFile changes(base.filePath(QStringLiteral("docs/changes.txt")));
    if (changes.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString text = QString::fromUtf8(changes.readAll());
        static const QRegularExpression versionRe(
            QStringLiteral(R"((?:^|\n)v?([0-9]+(?:\.[0-9]+){1,3})[ \t]*(?=\r?$))"),
            QRegularExpression::MultilineOption);
        auto it = versionRe.globalMatch(text);
        while (it.hasNext())
            report.version = it.next().captured(1);
    }

    QFile script(base.filePath(QStringLiteral("blockcheck2.sh")));
    if (script.open(QIODevice::ReadOnly | QIODevice::Text)) {
        const QString text = QString::fromUtf8(script.readAll());
        containsAll(text,
                    {QStringLiteral("ENABLE_HTTP"),
                     QStringLiteral("ENABLE_HTTPS_TLS12"),
                     QStringLiteral("ENABLE_HTTPS_TLS13"),
                     QStringLiteral("ENABLE_HTTP3"),
                     QStringLiteral("REPEATS"),
                     QStringLiteral("SCANLEVEL"),
                     QStringLiteral("working strategy found for ipv")},
                    &report.missingComponents);

        static const QRegularExpression scanLevelsRe(
            QStringLiteral(R"(ask_list\s+SCANLEVEL\s+"([^"]+)")"));
        const auto match = scanLevelsRe.match(text);
        const QStringList requiredLevels{
            QStringLiteral("veryquick"), QStringLiteral("quick"),
            QStringLiteral("standard"), QStringLiteral("force")
        };
        if (!match.hasMatch()) {
            report.missingComponents.append(QStringLiteral("blockcheck2.sh: SCANLEVEL choices"));
        } else {
            const QStringList available = match.captured(1).split(
                QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
            for (const QString &level : requiredLevels) {
                if (!available.contains(level))
                    report.missingComponents.append(
                        QStringLiteral("blockcheck2.sh: scan level %1").arg(level));
            }
        }
    } else if (!report.missingComponents.contains(QStringLiteral("blockcheck2.sh"))) {
        report.missingComponents.append(QStringLiteral("blockcheck2.sh: unreadable"));
    }

    report.compatible = report.missingComponents.isEmpty() && !report.version.isEmpty();
    if (report.version.isEmpty())
        report.missingComponents.append(QStringLiteral("docs/changes.txt: version not found"));
    return report;
}

} // namespace ZapretCompatibility
