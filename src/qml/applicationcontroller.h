#pragma once

#include <QObject>
#include <QString>

#include "backend/zapretbackend.h"

class SystemdBackend;

class ApplicationController final : public QObject
{
    Q_OBJECT
    Q_PROPERTY(QString serviceState READ serviceState NOTIFY serviceStateChanged)
    Q_PROPERTY(bool serviceActive READ serviceActive NOTIFY serviceStateChanged)
    Q_PROPERTY(QString serviceStateCode READ serviceStateCode NOTIFY serviceStateChanged)
    Q_PROPERTY(bool bootEnabled READ bootEnabled NOTIFY bootEnabledChanged)
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit ApplicationController(QObject *parent = nullptr);

    QString serviceState() const;
    bool serviceActive() const;
    QString serviceStateCode() const;
    bool bootEnabled() const;
    QString errorMessage() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void startService();
    Q_INVOKABLE void stopService();
    Q_INVOKABLE void setBootEnabled(bool enabled);

signals:
    void serviceStateChanged();
    void errorMessageChanged();
    void bootEnabledChanged();

private:
    void setState(ZapretBackend::State state);

    SystemdBackend *m_backend = nullptr;
    ZapretBackend::State m_state = ZapretBackend::State::Unknown;
    bool m_bootEnabled = false;
    QString m_errorMessage;
};
