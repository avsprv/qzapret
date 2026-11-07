#include "initbackends.h"

#include <QFile>
#include <QFileInfo>
#include <QProcess>
#include <QStandardPaths>
#include <QStringList>
#include <unistd.h>

#include "helperpath.h"

namespace {
bool hasExecutable(const QString &name)
{
    return !QStandardPaths::findExecutable(name).isEmpty() ||
           QFileInfo(QStringLiteral("/sbin/") + name).isExecutable() ||
           QFileInfo(QStringLiteral("/usr/sbin/") + name).isExecutable();
}
}

InitScriptBackend::InitScriptBackend(QString backendId, QString name, QObject *parent)
    : ZapretBackend(parent)
    , m_backendId(std::move(backendId))
    , m_name(std::move(name))
{
}

QString InitScriptBackend::displayName() const
{
    return m_name;
}

bool InitScriptBackend::isAvailable() const
{
    if (geteuid() != 0 && QStandardPaths::findExecutable(QStringLiteral("pkexec")).isEmpty())
        return false;

    const QString servicePath = QStringLiteral("/etc/init.d/zapret2");
    const QString upstreamPath = QStringLiteral("/opt/zapret2/init.d/%1/zapret2").arg(m_backendId);
    const QFileInfo serviceInfo(servicePath);
    const QFileInfo upstreamInfo(upstreamPath);
    if (!serviceInfo.isFile() || !serviceInfo.isExecutable() ||
        !upstreamInfo.isFile() || !upstreamInfo.isExecutable())
        return false;

    QFile installed(servicePath);
    QFile upstream(upstreamPath);
    if (!installed.open(QIODevice::ReadOnly) || !upstream.open(QIODevice::ReadOnly) ||
        installed.readAll() != upstream.readAll())
        return false;

    if (m_backendId == QLatin1String("openrc"))
        return hasExecutable(QStringLiteral("rc-service")) &&
               hasExecutable(QStringLiteral("rc-update"));

    return QFileInfo::exists(QStringLiteral("/etc/rc.common")) &&
           hasExecutable(QStringLiteral("ubus"));
}

void InitScriptBackend::queryState()
{
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc](int code, QProcess::ExitStatus status) {
        const QString output = status == QProcess::NormalExit && code == 0
            ? QString::fromUtf8(proc->readAllStandardOutput()).trimmed() : QString();
        const QStringList values = output.split(QLatin1Char('\n'));
        State state = State::Unknown;
        if (!values.isEmpty() && values.at(0) == QLatin1String("active"))
            state = State::Active;
        else if (!values.isEmpty() && values.at(0) == QLatin1String("inactive"))
            state = State::Inactive;
        emit stateChanged(state);
        emit bootEnabledChanged(values.size() > 1 && values.at(1) == QLatin1String("enabled"));
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        emit stateChanged(State::Unknown);
        emit bootEnabledChanged(false);
        proc->deleteLater();
    });
    Helper::start(proc, {QStringLiteral("service"), m_backendId, QStringLiteral("state")});
}

void InitScriptBackend::refresh()
{
    if (!isAvailable()) {
        emit stateChanged(State::Unknown);
        emit bootEnabledChanged(false);
        return;
    }
    queryState();
}

void InitScriptBackend::start()
{
    runAction(QStringLiteral("start"));
}

void InitScriptBackend::stop()
{
    runAction(QStringLiteral("stop"));
}

void InitScriptBackend::setBootEnabled(bool enabled)
{
    runAction(enabled ? QStringLiteral("enable") : QStringLiteral("disable"));
}

void InitScriptBackend::runAction(const QString &action)
{
    if (!isAvailable()) {
        emit errorOccurred(tr("Не найден корректный upstream init-скрипт zapret2 для %1.").arg(m_name));
        return;
    }
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc](int code, QProcess::ExitStatus) {
        if (code != 0 && !Helper::isAuthDismissed(code)) {
            const QString error = QString::fromUtf8(proc->readAllStandardError()).trimmed();
            emit errorOccurred(tr("Операция init-системы не выполнена (код %1): %2").arg(code).arg(error));
        }
        proc->deleteLater();
        refresh();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error == QProcess::FailedToStart) {
            emit errorOccurred(tr("Не удалось запустить pkexec/хелпер: %1").arg(proc->errorString()));
            proc->deleteLater();
        }
    });
    Helper::start(proc, {QStringLiteral("service"), m_backendId, action});
}
