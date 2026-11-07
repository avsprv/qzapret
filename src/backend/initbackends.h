#pragma once
#include <functional>
#include <utility>
#include "zapretbackend.h"

class InitScriptBackend : public ZapretBackend
{
    Q_OBJECT
public:
    QString displayName() const override;
    bool isAvailable() const override;
    int refreshIntervalMs() const override { return 10000; }
    void refresh() override;
    void start() override;
    void stop() override;
    void setBootEnabled(bool enabled) override;
protected:
    explicit InitScriptBackend(QString backendId, QString name, QObject *parent = nullptr);
private:
    void queryState();
    void runAction(const QString &action);
    QString m_backendId;
    QString m_name;
};

class OpenRCBackend final : public InitScriptBackend
{
public:
    explicit OpenRCBackend(QObject *parent = nullptr)
        : InitScriptBackend(QStringLiteral("openrc"), QStringLiteral("OpenRC"), parent) {}
};

class OpenWrtBackend final : public InitScriptBackend
{
public:
    explicit OpenWrtBackend(QObject *parent = nullptr)
        : InitScriptBackend(QStringLiteral("openwrt"), QStringLiteral("OpenWrt procd"), parent) {}
};
