#include "backendfactory.h"

#include <QFileInfo>
#include <QStandardPaths>

#include "initbackends.h"
#include "systemdbackend.h"
#include "zapretbackend.h"

namespace {
class UnsupportedBackend final : public ZapretBackend
{
public:
    explicit UnsupportedBackend(QObject *parent = nullptr) : ZapretBackend(parent) {}
    QString displayName() const override { return QStringLiteral("Unsupported"); }
    bool isAvailable() const override { return false; }
    void refresh() override {
        emit stateChanged(State::Unknown);
        emit bootEnabledChanged(false);
    }
    void start() override { emit errorOccurred(tr("Не обнаружена поддерживаемая init-система для zapret2.")); }
    void stop() override { emit errorOccurred(tr("Не обнаружена поддерживаемая init-система для zapret2.")); }
    void setBootEnabled(bool) override { emit errorOccurred(tr("Не обнаружена поддерживаемая init-система для zapret2.")); }
};

bool hasExecutable(const QString &name)
{
    return !QStandardPaths::findExecutable(name).isEmpty() ||
           QFileInfo(QStringLiteral("/sbin/") + name).isExecutable() ||
           QFileInfo(QStringLiteral("/usr/sbin/") + name).isExecutable();
}
}

ZapretBackend *BackendFactory::create(QObject *parent)
{
    if (QFileInfo::exists(QStringLiteral("/etc/openwrt_release")) ||
        QFileInfo::exists(QStringLiteral("/etc/openwrt_version")))
        return new OpenWrtBackend(parent);

    if (QFileInfo::exists(QStringLiteral("/run/systemd/system")))
        return new SystemdBackend(parent);

    if (QFileInfo::exists(QStringLiteral("/run/openrc")) ||
        (hasExecutable(QStringLiteral("rc-service")) && hasExecutable(QStringLiteral("rc-update"))))
        return new OpenRCBackend(parent);

    if (hasExecutable(QStringLiteral("systemctl")))
        return new SystemdBackend(parent);

    return new UnsupportedBackend(parent);
}
