#include <QtTest>
#include <QProcess>
#include <QSignalSpy>

#include "backend/systemdbackend.h"

class TestSystemdBackend final : public SystemdBackend
{
    Q_OBJECT
public:
    using SystemdBackend::SystemdBackend;

    QStringList actions;
    int activePrivilegedProcesses = 0;
    int maxConcurrentPrivilegedProcesses = 0;
    bool failNextActionToStart = false;
    bool failNextActionToExecute = false;
    bool failNextStateQueryToStart = false;

protected:
    void startProcess(QProcess *process, const QString &program,
                      const QStringList &arguments, bool privileged) override
    {
        Q_UNUSED(program);
        if (privileged) {
            const QString action = arguments.value(1);
            actions.append(action);
            ++activePrivilegedProcesses;
            maxConcurrentPrivilegedProcesses = qMax(maxConcurrentPrivilegedProcesses,
                                                       activePrivilegedProcesses);
            connect(process, &QProcess::finished, this, [this] { --activePrivilegedProcesses; });
            connect(process, &QProcess::errorOccurred, this,
                    [this](QProcess::ProcessError error) {
                if (error == QProcess::FailedToStart)
                    --activePrivilegedProcesses;
            });
            if (failNextActionToStart) {
                failNextActionToStart = false;
                process->start(QStringLiteral("/path/that/does/not/exist"));
                return;
            }
            if (failNextActionToExecute) {
                failNextActionToExecute = false;
                process->start(QStringLiteral("/bin/sh"),
                               {QStringLiteral("-c"), QStringLiteral("exit 1")});
                return;
            }
            process->start(QStringLiteral("/bin/sh"),
                           {QStringLiteral("-c"), QStringLiteral("exit 0")});
            return;
        }

        if (arguments.value(0) == QLatin1String("is-active") && failNextStateQueryToStart) {
            failNextStateQueryToStart = false;
            process->start(QStringLiteral("/path/that/does/not/exist"));
            return;
        }

        const QString output = arguments.value(0) == QLatin1String("is-enabled")
                                   ? QStringLiteral("enabled")
                                   : QStringLiteral("active");
        process->start(QStringLiteral("/bin/sh"),
                       {QStringLiteral("-c"), QStringLiteral("printf '%1'").arg(output)});
    }
};

class SystemdBackendTest final : public QObject
{
    Q_OBJECT

private slots:
    void serializesUnitActionsInFifoOrder();
    void continuesAfterFailedToStart();
    void continuesAfterNonzeroExit();
    void releasesQueueWhenStateQueryCannotStart();
    void correlatedVerificationDoesNotPublishGlobalState();
    void ordinaryVerificationPublishesGlobalState();
};

void SystemdBackendTest::serializesUnitActionsInFifoOrder()
{
    TestSystemdBackend backend;
    QSignalSpy finished(&backend, &ZapretBackend::unitActionFinished);

    backend.stop();
    backend.start();
    backend.setBootEnabled(false);
    backend.setBootEnabled(true);

    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 4, 3000);
    QCOMPARE(backend.actions,
             QStringList({QStringLiteral("stop"), QStringLiteral("start"),
                          QStringLiteral("disable"), QStringLiteral("enable")}));
    QCOMPARE(backend.maxConcurrentPrivilegedProcesses, 1);
}

void SystemdBackendTest::continuesAfterFailedToStart()
{
    TestSystemdBackend backend;
    backend.failNextActionToStart = true;
    QSignalSpy finished(&backend, &ZapretBackend::unitActionFinished);

    backend.stop();
    backend.start();

    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 2, 3000);
    QCOMPARE(backend.actions,
             QStringList({QStringLiteral("stop"), QStringLiteral("start")}));
    QVERIFY(!finished.at(0).at(2).toBool());
    QVERIFY(finished.at(1).at(2).toBool());
}

void SystemdBackendTest::continuesAfterNonzeroExit()
{
    TestSystemdBackend backend;
    backend.failNextActionToExecute = true;
    QSignalSpy finished(&backend, &ZapretBackend::unitActionFinished);

    backend.stop();
    backend.start();

    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 2, 3000);
    QVERIFY(!finished.at(0).at(2).toBool());
    QVERIFY(finished.at(1).at(2).toBool());
    QCOMPARE(backend.actions.size(), 2);
}

void SystemdBackendTest::correlatedVerificationDoesNotPublishGlobalState()
{
    TestSystemdBackend backend;
    QSignalSpy globalState(&backend, &ZapretBackend::stateChanged);
    QSignalSpy verified(&backend, &ZapretBackend::stateVerifiedForRequest);

    backend.verifyState(quint64(42));

    QTRY_COMPARE_WITH_TIMEOUT(verified.count(), 1, 3000);
    QCOMPARE(verified.at(0).at(0).toULongLong(), quint64(42));
    QCOMPARE(globalState.count(), 0);
}

void SystemdBackendTest::ordinaryVerificationPublishesGlobalState()
{
    TestSystemdBackend backend;
    QSignalSpy globalState(&backend, &ZapretBackend::stateChanged);
    QSignalSpy verified(&backend, &ZapretBackend::stateVerified);

    backend.verifyState();

    QTRY_COMPARE_WITH_TIMEOUT(verified.count(), 1, 3000);
    QCOMPARE(globalState.count(), 1);
    QCOMPARE(globalState.at(0).at(0).value<ZapretBackend::State>(), ZapretBackend::State::Active);
}

void SystemdBackendTest::releasesQueueWhenStateQueryCannotStart()
{
    TestSystemdBackend backend;
    backend.failNextStateQueryToStart = true;
    QSignalSpy finished(&backend, &ZapretBackend::unitActionFinished);

    backend.stop();
    backend.start();

    QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 2, 3000);
    QCOMPARE(backend.actions.size(), 2);
}

QTEST_GUILESS_MAIN(SystemdBackendTest)
#include "test_systemdbackend.moc"
