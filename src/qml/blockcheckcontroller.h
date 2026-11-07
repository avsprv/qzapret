#pragma once

#include <QObject>
#include <QVariantList>
#include <QString>
#include <QUrl>
#include "backend/blockcheckrunner.h"
#include "backend/zapretbackend.h"

class SystemdBackend;
class QTimer;

class BlockcheckController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString domains READ domains WRITE setDomains NOTIFY optionsChanged)
    Q_PROPERTY(QString ipVersion READ ipVersion WRITE setIpVersion NOTIFY optionsChanged)
    Q_PROPERTY(bool http READ http WRITE setHttp NOTIFY optionsChanged)
    Q_PROPERTY(bool tls12 READ tls12 WRITE setTls12 NOTIFY optionsChanged)
    Q_PROPERTY(bool tls13 READ tls13 WRITE setTls13 NOTIFY optionsChanged)
    Q_PROPERTY(bool http3 READ http3 WRITE setHttp3 NOTIFY optionsChanged)
    Q_PROPERTY(int repeats READ repeats WRITE setRepeats NOTIFY optionsChanged)
    Q_PROPERTY(QString scanLevel READ scanLevel WRITE setScanLevel NOTIFY optionsChanged)
    Q_PROPERTY(QString logText READ logText NOTIFY logTextChanged)
    Q_PROPERTY(QVariantList results READ results NOTIFY resultsChanged)
    Q_PROPERTY(QString status READ status NOTIFY statusChanged)
    Q_PROPERTY(bool busy READ busy NOTIFY busyChanged)
    Q_PROPERTY(bool updateBusy READ updateBusy WRITE setUpdateBusy NOTIFY updateBusyChanged)

public:
    explicit BlockcheckController(QObject *parent = nullptr);
    QString domains() const; void setDomains(const QString &value);
    QString ipVersion() const; void setIpVersion(const QString &value);
    bool http() const; void setHttp(bool value);
    bool tls12() const; void setTls12(bool value);
    bool tls13() const; void setTls13(bool value);
    bool http3() const; void setHttp3(bool value);
    int repeats() const; void setRepeats(int value);
    QString scanLevel() const; void setScanLevel(const QString &value);
    QString logText() const;
    QVariantList results() const;
    QString status() const;
    bool busy() const;
    bool updateBusy() const;
    void setUpdateBusy(bool value);

    Q_INVOKABLE void start();
    Q_INVOKABLE void confirmStop(bool accepted);
    Q_INVOKABLE void cancel();
    Q_INVOKABLE void copyStrategy(int index);
    Q_INVOKABLE bool saveLog(const QUrl &url);

signals:
    void optionsChanged();
    void logTextChanged();
    void resultsChanged();
    void statusChanged();
    void busyChanged();
    void updateBusyChanged();
    void confirmationRequired(const QString &title, const QString &message);
    void scanFinished(int found, bool canceled);

private:
    enum class Phase { Idle, Checking, AwaitingConfirmation, WaitingForStop, Scanning, WaitingForStart };
    void appendLog(const QString &line);
    void setStatus(const QString &value);
    void setBusy(bool value);
    void finish(const QString &message = {});
    void beginScan();
    void restoreService(const QString &reason);
    void handleVerifiedState(ZapretBackend::State state);
    void handleUnitAction(const QString &action, ZapretBackend::State state, bool success);
    void handleFinished(int exitCode, bool canceled);
    BlockcheckRunner::Options collectOptions() const;

    QTimer *m_lifecycleTimeout = nullptr;
    SystemdBackend *m_backend = nullptr;
    BlockcheckRunner *m_runner = nullptr;
    Phase m_phase = Phase::Idle;
    QString m_domains = QStringLiteral("rutracker.org");
    QString m_ipVersion = QStringLiteral("4");
    bool m_http = true, m_tls12 = true, m_tls13 = false, m_http3 = false;
    int m_repeats = 1;
    QString m_scanLevel = QStringLiteral("standard");
    QString m_logText, m_status;
    QVariantList m_results;
    bool m_busy = false, m_updateBusy = false;
    bool m_resumeService = false, m_cancelRequested = false;
    int m_found = 0;
    BlockcheckRunner::Options m_lastOptions;
};
