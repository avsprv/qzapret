#include "systemdbackend.h"

#include <QProcess>

#include "helperpath.h"

SystemdBackend::SystemdBackend(QObject *parent)
    : ZapretBackend(parent)
    , m_unit(QStringLiteral("zapret2.service"))
{
}

void SystemdBackend::query(const QString &verb, const std::function<void(const QString &)> &onResult)
{
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [proc, onResult] {
        onResult(QString::fromUtf8(proc->readAllStandardOutput()).trimmed());
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc, onResult] {
        emit errorOccurred(tr("Не удалось запустить systemctl: %1").arg(proc->errorString()));
        onResult(QString());
        proc->deleteLater();
    });
    proc->start(QStringLiteral("systemctl"), {verb, m_unit});
}

namespace {
ZapretBackend::State parseUnitState(const QString &out)
{
    if (out == QLatin1String("active"))
        return ZapretBackend::State::Active;
    if (out == QLatin1String("inactive"))
        return ZapretBackend::State::Inactive;
    if (out == QLatin1String("failed"))
        return ZapretBackend::State::Failed;
    return ZapretBackend::State::Unknown;
}
} // namespace

void SystemdBackend::refresh()
{
    query(QStringLiteral("is-active"), [this](const QString &out) {
        emit stateChanged(parseUnitState(out));
    });
    query(QStringLiteral("is-enabled"), [this](const QString &out) {
        emit bootEnabledChanged(out == QLatin1String("enabled"));
    });
}

void SystemdBackend::verifyState()
{
    query(QStringLiteral("is-active"), [this](const QString &out) {
        const State state = parseUnitState(out);
        emit stateChanged(state);
        emit stateVerified(state);
    });
}

void SystemdBackend::start() { runUnitAction(QStringLiteral("start")); }
void SystemdBackend::stop() { runUnitAction(QStringLiteral("stop")); }

void SystemdBackend::setBootEnabled(bool enabled)
{
    runUnitAction(enabled ? QStringLiteral("enable") : QStringLiteral("disable"));
}

void SystemdBackend::runUnitAction(const QString &action)
{
    auto *proc = new QProcess(this);
    const auto reportResult = [this, action](bool success) {
        query(QStringLiteral("is-active"), [this, action, success](const QString &out) {
            const State state = parseUnitState(out);
            emit stateChanged(state);
            emit unitActionFinished(action, state, success);
        });
        query(QStringLiteral("is-enabled"), [this](const QString &out) {
            emit bootEnabledChanged(out == QLatin1String("enabled"));
        });
    };

    connect(proc, &QProcess::finished, this,
            [this, proc, action, reportResult](int code, QProcess::ExitStatus status) {
        const bool success = code == 0 && status == QProcess::NormalExit;
        if (!success && !Helper::isAuthDismissed(code)) {
            const QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
            emit errorOccurred(tr("Операция не выполнена (код %1): %2").arg(code).arg(err));
        }
        proc->deleteLater();
        reportResult(success);
    });
    connect(proc, &QProcess::errorOccurred, this,
            [this, proc, reportResult](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            emit errorOccurred(tr("Не удалось запустить pkexec/хелпер: %1").arg(proc->errorString()));
            proc->deleteLater(); // finished() после FailedToStart не приходит
            reportResult(false);
        }
    });
    Helper::start(proc, {QStringLiteral("unit"), action});
}
