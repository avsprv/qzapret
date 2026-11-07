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
    Q_PROPERTY(QString errorMessage READ errorMessage NOTIFY errorMessageChanged)

public:
    explicit ApplicationController(QObject *parent = nullptr);

    QString serviceState() const;
    bool serviceActive() const;
    QString errorMessage() const;

    Q_INVOKABLE void refresh();
    Q_INVOKABLE void startService();
    Q_INVOKABLE void stopService();

signals:
    void serviceStateChanged();
    void errorMessageChanged();

private:
    void setState(ZapretBackend::State state);

    SystemdBackend *m_backend = nullptr;
    ZapretBackend::State m_state = ZapretBackend::State::Unknown;
    QString m_errorMessage;
};
