#include "systemdbackend.h"

#include <QProcess>

SystemdBackend::SystemdBackend(QObject *parent)
    : ZapretBackend(parent)
    , m_unit(QStringLiteral("zapret2.service"))
{
}

void SystemdBackend::refresh()
{
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc] {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        State s = State::Unknown;
        if (out == QLatin1String("active"))
            s = State::Active;
        else if (out == QLatin1String("inactive"))
            s = State::Inactive;
        else if (out == QLatin1String("failed"))
            s = State::Failed;
        emit stateChanged(s);
        proc->deleteLater();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc] {
        emit errorOccurred(tr("Не удалось запустить systemctl: %1").arg(proc->errorString()));
        emit stateChanged(State::Unknown);
        proc->deleteLater();
    });
    proc->start(QStringLiteral("systemctl"), {QStringLiteral("is-active"), m_unit});
}

void SystemdBackend::start()
{
    runSystemctl({QStringLiteral("start"), m_unit}, true);
}

void SystemdBackend::stop()
{
    runSystemctl({QStringLiteral("stop"), m_unit}, true);
}

void SystemdBackend::runSystemctl(const QStringList &args, bool privileged)
{
    auto *proc = new QProcess(this);
    connect(proc, &QProcess::finished, this, [this, proc](int code, QProcess::ExitStatus) {
        if (code != 0) {
            const QString err = QString::fromUtf8(proc->readAllStandardError()).trimmed();
            emit errorOccurred(tr("systemctl завершился с кодом %1: %2").arg(code).arg(err));
        }
        proc->deleteLater();
        refresh();
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError) {
        emit errorOccurred(tr("Не удалось запустить процесс: %1").arg(proc->errorString()));
        proc->deleteLater();
    });

    if (privileged)
        proc->start(QStringLiteral("pkexec"), QStringList{QStringLiteral("systemctl")} + args);
    else
        proc->start(QStringLiteral("systemctl"), args);
}
