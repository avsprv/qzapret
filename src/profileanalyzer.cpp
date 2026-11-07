#include "profileanalyzer.h"
#include <QCoreApplication>
#include <QRegularExpression>
namespace ProfileAnalyzer {
namespace {
QString errorText(const char *source)
{
    return QCoreApplication::translate("ProfilePage", source);
}
bool findAssignment(const QString &config, qsizetype *quoteStart, qsizetype *quoteEnd,
                    QString *error)
{
    static const QRegularExpression assignment(
        QStringLiteral("^[ \\t]*NFQWS2_OPT[ \\t]*="),
        QRegularExpression::MultilineOption);
    auto matches = assignment.globalMatch(config);
    if (!matches.hasNext()) {
        *error = errorText("NFQWS2_OPT не найден или задан несколько раз.");
        return false;
    }
    const auto match = matches.next();
    if (matches.hasNext()) {
        *error = errorText("NFQWS2_OPT не найден или задан несколько раз.");
        return false;
    }
    qsizetype pos = match.capturedEnd();
    while (pos < config.size() && (config.at(pos) == QLatin1Char(' ') ||
           config.at(pos) == QLatin1Char('\t')))
        ++pos;
    if (pos >= config.size() || config.at(pos) != QLatin1Char('"')) {
        *error = errorText("NFQWS2_OPT должен быть статической строкой в двойных кавычках.");
        return false;
    }
    *quoteStart = pos;
    ++pos;
    for (; pos < config.size(); ++pos) {
        const QChar ch = config.at(pos);
        if (ch == QLatin1Char('"')) {
            *quoteEnd = pos;
            ++pos;
            break;
        }
        if (ch == QLatin1Char('$') || ch == QChar(96) ||
            ch == QLatin1Char('\\') || ch.isNull()) {
            *error = errorText("NFQWS2_OPT содержит shell-подстановку или escape-последовательность; безопасный статический анализ невозможен.");
            return false;
        }
    }
    if (*quoteEnd < 0) {
        *error = errorText("NFQWS2_OPT должен быть статической строкой в двойных кавычках.");
        return false;
    }
    while (pos < config.size() && (config.at(pos) == QLatin1Char(' ') ||
           config.at(pos) == QLatin1Char('\t') || config.at(pos) == QLatin1Char('\r')))
        ++pos;
    if (pos < config.size() && config.at(pos) != QLatin1Char('\n') &&
        config.at(pos) != QLatin1Char('#')) {
        *error = errorText("После закрывающей кавычки NFQWS2_OPT обнаружено неподдерживаемое выражение.");
        return false;
    }
    return true;
}
bool validArgument(const QString &argument)
{
    if (argument.isEmpty())
        return false;
    for (const QChar ch : argument) {
        if (ch.isSpace() || ch.category() == QChar::Other_Control || ch == QLatin1Char('"') ||
            ch == QLatin1Char('$') || ch == QChar(96) || ch == QLatin1Char('\\'))
            return false;
    }
    return true;
}
}
Result parse(const QString &config)
{
    Result result;
    qsizetype quoteStart = -1;
    qsizetype quoteEnd = -1;
    if (!findAssignment(config, &quoteStart, &quoteEnd, &result.error))
        return result;
    const QString value = config.mid(quoteStart + 1, quoteEnd - quoteStart - 1);
    const QStringList arguments = value.split(
        QRegularExpression(QStringLiteral("\\s+")), Qt::SkipEmptyParts);
    if (arguments.isEmpty()) {
        result.error = errorText("NFQWS2_OPT не содержит параметров.");
        return result;
    }
    if (arguments.size() > 4096) {
        result.error = errorText("Слишком много профилей или параметров.");
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
                result.error = result.profiles.size() > 128
                    ? errorText("Слишком много профилей или параметров.")
                    : errorText("Обнаружен пустой профиль рядом с --new.");
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
        result.error = errorText("Обнаружен пустой профиль рядом с --new.");
        result.profiles.clear();
    } else if (result.profiles.size() > 128) {
        result.error = errorText("Слишком много профилей или параметров.");
        result.profiles.clear();
    }
    return result;
}
EditResult replaceProfiles(const QString &config, const QVector<Profile> &profiles)
{
    EditResult result;
    const Result original = parse(config);
    if (!original.ok()) {
        result.error = original.error;
        return result;
    }
    if (profiles.isEmpty() || profiles.size() > 128) {
        result.error = errorText("Количество профилей должно быть от 1 до 128.");
        return result;
    }
    QStringList serialized;
    qsizetype argumentCount = 0;
    for (const Profile &profile : profiles) {
        if (profile.allArguments.isEmpty()) {
            result.error = errorText("Профиль не может быть пустым.");
            return result;
        }
        for (const QString &argument : profile.allArguments) {
            if (!validArgument(argument)) {
                result.error = errorText("Параметр содержит пробелы или shell-символы, которые нельзя безопасно сериализовать.");
                return result;
            }
            serialized.append(argument);
            ++argumentCount;
            if (argumentCount > 4096) {
                result.error = errorText("Слишком много профилей или параметров.");
                return result;
            }
        }
        if (&profile != &profiles.last())
            serialized.append(QStringLiteral("--new"));
    }
    const QString value = serialized.join(QLatin1Char(' '));
    if (value.toUtf8().size() > 1024 * 1024) {
        result.error = errorText("Конфигурация превышает ограничение 1 MiB.");
        return result;
    }
    qsizetype quoteStart = -1;
    qsizetype quoteEnd = -1;
    if (!findAssignment(config, &quoteStart, &quoteEnd, &result.error))
        return result;
    result.config = config.left(quoteStart + 1) + value + config.mid(quoteEnd);
    return result;
}
}
