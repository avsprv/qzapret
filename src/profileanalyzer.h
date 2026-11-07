#pragma once
#include <QString>
#include <QStringList>
#include <QVector>
namespace ProfileAnalyzer {
struct Profile {
    QStringList filters;
    QStringList payloads;
    QStringList luaDesync;
    QStringList other;
    QStringList allArguments;
};
struct Result {
    QVector<Profile> profiles;
    QString error;
    bool ok() const { return error.isEmpty() && !profiles.isEmpty(); }
};
struct EditResult {
    QString config;
    QString error;
    bool ok() const { return error.isEmpty() && !config.isNull(); }
};
Result parse(const QString &config);
// Replaces only the static NFQWS2_OPT value. All other config bytes are preserved.
EditResult replaceProfiles(const QString &config, const QVector<Profile> &profiles);
}
