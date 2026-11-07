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
Result parse(const QString &config);
}
