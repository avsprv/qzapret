#pragma once

#include <QStringList>
#include <functional>

#include "zapretbackend.h"

// Управляет апстримным юнитом zapret2.service.
// Чтение статуса — без привилегий; start/stop/enable/disable — через zapret2applet-helper (polkit).
class SystemdBackend : public ZapretBackend
{
    Q_OBJECT
public:
    explicit SystemdBackend(QObject *parent = nullptr);

    void refresh() override;
    void verifyState() override;
    void start() override;
    void stop() override;
    void setBootEnabled(bool enabled) override;

private:
    void runUnitAction(const QString &action);
    void query(const QString &verb, const std::function<void(const QString &)> &onResult);

    QString m_unit;
};
