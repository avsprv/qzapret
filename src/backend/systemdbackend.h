#pragma once

#include <QStringList>

#include "zapretbackend.h"

// Управляет апстримным юнитом zapret2.service через systemctl.
// Чтение статуса идёт без привилегий, start/stop — через pkexec (polkit).
class SystemdBackend : public ZapretBackend
{
    Q_OBJECT
public:
    explicit SystemdBackend(QObject *parent = nullptr);

    void refresh() override;
    void start() override;
    void stop() override;

private:
    void runSystemctl(const QStringList &args, bool privileged);

    QString m_unit;
};
