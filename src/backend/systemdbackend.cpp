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

void SystemdBackend::refresh()
{
    query(QStringLiteral("is-active"), [this](const QString &out) {
        State s = State::Unknown;
        if (out == QLatin1String("active"))
            s = State::Active;
        else if (out == QLatin1String("inactive"))
            s = State::Inactive;
        else if (out == QLatin1String("failed"))
            s = State::Failed;
        emit stateChanged(s);
    });
    query(QStringLiteral("is-enabled"), [this](const QString &out) {
        emit bootEnabledChanged(out == QLatin1String("enabled"));
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
    connect(proc, &QProcess::finished, this, [this, proc](int code, QProcess::ExitStatus) {
        if (code != 0 && !Helper::isAuthDismissed(code)) {
            const QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
            emit errorOccurred(tr("Операция не выполнена (код %1): %2").arg(code).arg(err));
        }
        proc->deleteLater();
        refresh();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError e) {
        if (e == QProcess::FailedToStart) {
            emit errorOccurred(tr("Не удалось запустить pkexec/хелпер: %1").arg(proc->errorString()));
            proc->deleteLater(); // finished() после FailedToStart не приходит
            refresh(); // сообщить странице фактическое состояние после неудачной команды
        }
    });
    Helper::start(proc, {QStringLiteral("unit"), action});
}
