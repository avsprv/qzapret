#pragma once

#include <QObject>
#include <QProcess>
#include <QSet>
#include <QString>
#include <QStringList>
#include <optional>

// Запускает blockcheck2.sh из апстрима (через zapret2applet-helper) в batch-режиме
// и разбирает найденные стратегии из его вывода.
class BlockcheckRunner : public QObject
{
    Q_OBJECT
public:
    struct Options {
        QStringList domains{QStringLiteral("rutracker.org")};
        QString ipvs = QStringLiteral("4"); // "4", "6" или "46"
        bool http = true;
        bool tls12 = true;
        bool tls13 = false;
        bool http3 = false;
        int repeats = 1;
        QString scanLevel = QStringLiteral("standard"); // veryquick|quick|standard|force
    };

    struct Result {
        QString test;   // например curl_test_https_tls12
        QString ipv;    // "4" | "6"
        QString domain;
        QString daemon; // nfqws2
        QString args;   // аргументы стратегии
        QString commandLine() const { return daemon + QLatin1Char(' ') + args; }
        bool operator==(const Result &) const = default;
    };

    explicit BlockcheckRunner(QObject *parent = nullptr);

    // Разбор строки вида:
    // "!!!!! curl_test_https_tls12: working strategy found for ipv4 example.org : nfqws2 --opt !!!!!"
    static std::optional<Result> parseLine(const QString &line);

    bool isRunning() const;
    void start(const Options &opt);
    void cancel();

signals:
    void logLine(const QString &line);
    void strategyFound(const BlockcheckRunner::Result &result);
    // exitCode: код завершения; canceled: пользователь отменил или закрыл диалог polkit
    void finished(int exitCode, bool canceled);

private:
    void onReadyRead();
    void handleLine(const QString &line);

    QProcess *m_proc = nullptr;
    QByteArray m_buf;
    QSet<QString> m_seenResults;
    bool m_cancelRequested = false;
};
