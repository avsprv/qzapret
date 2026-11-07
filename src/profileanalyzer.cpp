#include "profileanalyzer.h"
#include <QObject>
#include <QRegularExpression>
namespace ProfileAnalyzer {
Result parse(const QString &config)
{
    Result result;
    static const QRegularExpression assignment(
        QStringLiteral("^[ \\t]*NFQWS2_OPT[ \\t]*="),
        QRegularExpression::MultilineOption);
    auto matches = assignment.globalMatch(config);
    if (!matches.hasNext()) {
        result.error = QObject::tr("NFQWS2_OPT не найден или задан несколько раз.");
        return result;
    }
    const auto match = matches.next();
    if (matches.hasNext()) {
        result.error = QObject::tr("NFQWS2_OPT не найден или задан несколько раз.");
        return result;
    }
    qsizetype pos = match.capturedEnd();
    while (pos < config.size() && (config.at(pos) == QLatin1Char(' ') ||
           config.at(pos) == QLatin1Char('\t')))
        ++pos;
    if (pos >= config.size() || config.at(pos) != QLatin1Char('"')) {
        result.error = QObject::tr("NFQWS2_OPT должен быть статической строкой в двойных кавычках.");
        return result;
    }
    ++pos;
    QString value;
    bool closed = false;
    for (; pos < config.size(); ++pos) {
        const QChar ch = config.at(pos);
        if (ch == QLatin1Char('"')) {
            closed = true;
            ++pos;
            break;
        }
        if (ch == QLatin1Char('$') || ch == QChar(96) || ch == QLatin1Char('\\') || ch.isNull()) {
            result.error = QObject::tr("NFQWS2_OPT содержит shell-подстановку или escape-последовательность; безопасный статический анализ невозможен.");
            return result;
        }
        value.append(ch);
    }
    if (!closed) {
        result.error = QObject::tr("NFQWS2_OPT должен быть статической строкой в двойных кавычках.");
        return result;
    }
    while (pos < config.size() && (config.at(pos) == QLatin1Char(' ') ||
           config.at(pos) == QLatin1Char('\t') || config.at(pos) == QLatin1Char('\r')))
        ++pos;
    if (pos < config.size() && config.at(pos) != QLatin1Char('\n') &&
        config.at(pos) != QLatin1Char('#')) {
        result.error = QObject::tr("После закрывающей кавычки NFQWS2_OPT обнаружено неподдерживаемое выражение.");
        return result;
    }
    const QStringList arguments = value.split(
        QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (arguments.isEmpty()) {
        result.error = QObject::tr("NFQWS2_OPT не содержит параметров.");
        return result;
    }
    if (arguments.size() > 4096) {
        result.error = QObject::tr("Слишком много профилей или параметров.");
        return result;
    }
    Profile current;
    auto finishProfile = [&]() -> bool {
        if (current.allArguments.isEmpty())
            return false;
        result.profiles.append(current);
        current = Profile{};
        return result.profiles.size() <= 128;
    };
    for (const QString &argument : arguments) {
        if (argument == QStringLiteral("--new")) {
            if (!finishProfile()) {
                result.error = QObject::tr("Обнаружен пустой профиль рядом с --new.");
                result.profiles.clear();
                return result;
            }
            continue;
        }
        current.allArguments.append(argument);
        if (argument.startsWith(QStringLiteral("--filter-")))
            current.filters.append(argument);
        else if (argument.startsWith(QStringLiteral("--payload=")))
            current.payloads.append(argument);
        else if (argument.startsWith(QStringLiteral("--lua-desync=")))
            current.luaDesync.append(argument);
        else
            current.other.append(argument);
    }
    if (!finishProfile()) {
        result.error = QObject::tr("Обнаружен пустой профиль рядом с --new.");
        result.profiles.clear();
    } else if (result.profiles.size() > 128) {
        result.error = QObject::tr("Слишком много профилей или параметров.");
        result.profiles.clear();
    }
    return result;
}
}
