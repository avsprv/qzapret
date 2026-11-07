#include <QtTest>

#include "blockcheckcontroller.h"

class FakeZapretBackend final : public ZapretBackend
{
    Q_OBJECT
public:
    using ZapretBackend::ZapretBackend;

    State state = State::Inactive;
    bool autoVerify = true;
    bool autoStop = true;
    bool autoStart = true;
    int verifyCalls = 0;
    int stopCalls = 0;
    int startCalls = 0;
    quint64 lastRequestId = 0;

    void refresh() override {}
    void verifyState() override
    {
        ++verifyCalls;
        if (autoVerify)
            emit stateVerified(state);
    }
    void verifyState(quint64 requestId) override
    {
        lastRequestId = requestId;
        ++verifyCalls;
        if (autoVerify)
            emit stateVerifiedForRequest(requestId, state);
    }
    void start() override
    {
        ++startCalls;
        if (autoStart)
            completeStart(State::Active, true);
    }
    void start(quint64 requestId) override
    {
        lastRequestId = requestId;
        ++startCalls;
        if (autoStart)
            completeStart(requestId, State::Active, true);
    }
    void stop() override
    {
        ++stopCalls;
        if (autoStop)
            completeStop(State::Inactive, true);
    }
    void stop(quint64 requestId) override
    {
        lastRequestId = requestId;
        ++stopCalls;
        if (autoStop)
            completeStop(requestId, State::Inactive, true);
    }
    void setBootEnabled(bool) override {}

    void completeStop(State result, bool success)
    {
        state = result;
        emit unitActionFinished(QStringLiteral("stop"), state, success);
    }
    void completeStop(quint64 requestId, State result, bool success)
    {
        state = result;
        emit unitActionFinishedForRequest(requestId, QStringLiteral("stop"), state, success);
    }
    void completeStart(State result, bool success)
    {
        state = result;
        emit unitActionFinished(QStringLiteral("start"), state, success);
    }
    void completeStart(quint64 requestId, State result, bool success)
    {
        state = result;
        emit unitActionFinishedForRequest(requestId, QStringLiteral("start"), state, success);
    }
};

class FakeBlockcheckRunner final : public BlockcheckRunner
{
    Q_OBJECT
public:
    using BlockcheckRunner::BlockcheckRunner;

    bool autoFinish = true;
    int starts = 0;
    int cancels = 0;
    int errors = 0;
    int exitCode = 0;
    bool canceled = false;
    Options lastOptions;

    bool isRunning() const override { return starts > 0 && !finishedOnce; }
    int errorCount() const override { return errors; }
    void start(const Options &options) override
    {
        ++starts;
        lastOptions = options;
        finishedOnce = false;
        if (autoFinish)
            complete(exitCode, canceled);
    }
    void cancel() override
    {
        ++cancels;
        complete(130, true);
    }
    void complete(int code, bool wasCanceled)
    {
        if (finishedOnce)
            return;
        finishedOnce = true;
        emit finished(code, wasCanceled);
    }

private:
    bool finishedOnce = false;
};

class BlockcheckControllerTest final : public QObject
{
    Q_OBJECT

private slots:
    void clampsRepeatCount();
    void rejectsInvalidDomainBeforeStarting();
    void blocksScanDuringUpdate();
    void rejectsEmptyCheckSelection();
    void invalidInputHasErrorStatus();
    void busyGuardHasWarningStatus();
    void statusTypeChangesNotifyObservers();
    void statusSeverityPropertyMatchesEnum();
    void inactiveServiceRunsScanWithoutUnitActions();
    void activeServiceIsRestoredAfterSuccessfulScan();
    void decliningStopLeavesServiceUntouched();
    void cancelWhileStoppingRestoresServiceWithWarning();
    void scanFailureRemainsErrorAfterServiceRestore();
    void scanCancellationRemainsWarningAfterServiceRestore();
    void stopTimeoutWaitsForLateResultBeforeRestoring();
    void stopFailureAttemptsRecoveryAndWarns();
    void restoreFailureReportsError();
    void verificationTimeoutFinishesSafely();
    void restoreTimeoutReportsError();
    void ignoresLateVerificationFromPreviousOperation();
    void ignoresLateRestoreResultFromPreviousOperation();
};

void BlockcheckControllerTest::clampsRepeatCount()
{
    BlockcheckController controller;
    controller.setRepeats(0);
    QCOMPARE(controller.repeats(), 1);
    controller.setRepeats(100);
    QCOMPARE(controller.repeats(), 10);
}

void BlockcheckControllerTest::rejectsInvalidDomainBeforeStarting()
{
    BlockcheckController controller;
    controller.setDomains(QStringLiteral("example..org"));
    controller.start();
    QVERIFY(!controller.busy());
    QVERIFY(controller.status().contains(QStringLiteral("Некорректный домен")));
}

void BlockcheckControllerTest::blocksScanDuringUpdate()
{
    BlockcheckController controller;
    controller.setUpdateBusy(true);
    controller.start();
    QVERIFY(!controller.busy());
    QVERIFY(controller.status().contains(QStringLiteral("другой операции")));
}

void BlockcheckControllerTest::invalidInputHasErrorStatus()
{
    BlockcheckController controller;
    controller.setDomains(QStringLiteral("example..org"));
    controller.start();
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Error);
    QCOMPARE(controller.statusSeverity(), QStringLiteral("Error"));
}

void BlockcheckControllerTest::busyGuardHasWarningStatus()
{
    BlockcheckController controller;
    controller.setUpdateBusy(true);
    controller.start();
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Warning);
    QCOMPARE(controller.statusSeverity(), QStringLiteral("Warning"));
}

void BlockcheckControllerTest::statusTypeChangesNotifyObservers()
{
    BlockcheckController controller;
    QSignalSpy statusSpy(&controller, &BlockcheckController::statusChanged);
    controller.setDomains(QStringLiteral("example..org"));
    controller.start();
    QCOMPARE(statusSpy.count(), 1);
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Error);
}

void BlockcheckControllerTest::statusSeverityPropertyMatchesEnum()
{
    BlockcheckController controller;
    QCOMPARE(controller.statusSeverity(), QStringLiteral("Information"));
    controller.setDomains(QStringLiteral("example..org"));
    controller.start();
    QCOMPARE(controller.statusSeverity(), QStringLiteral("Error"));
}

void BlockcheckControllerTest::rejectsEmptyCheckSelection()
{
    BlockcheckController controller;
    controller.setHttp(false);
    controller.setTls12(false);
    controller.setTls13(false);
    controller.setHttp3(false);
    controller.start();
    QVERIFY(!controller.busy());
    QVERIFY(controller.status().contains(QStringLiteral("хотя бы один")));
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Error);
}

void BlockcheckControllerTest::inactiveServiceRunsScanWithoutUnitActions()
{
    FakeZapretBackend backend;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 100);

    QSignalSpy finished(&controller, &BlockcheckController::scanFinished);
    controller.start();

    QCOMPARE(backend.verifyCalls, 1);
    QCOMPARE(backend.stopCalls, 0);
    QCOMPARE(backend.startCalls, 0);
    QCOMPARE(runner.starts, 1);
    QCOMPARE(finished.count(), 1);
    QVERIFY(!controller.busy());
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Success);
}

void BlockcheckControllerTest::activeServiceIsRestoredAfterSuccessfulScan()
{
    FakeZapretBackend backend;
    backend.state = ZapretBackend::State::Active;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 100);

    QSignalSpy confirmation(&controller, &BlockcheckController::confirmationRequired);
    controller.start();
    QCOMPARE(confirmation.count(), 1);
    QCOMPARE(runner.starts, 0);

    controller.confirmStop(true);
    QCOMPARE(backend.stopCalls, 1);
    QCOMPARE(runner.starts, 1);
    QCOMPARE(backend.startCalls, 1);
    QVERIFY(!controller.busy());
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Success);
    QVERIFY(controller.status().contains(QStringLiteral("Служба восстановлена")));
}

void BlockcheckControllerTest::decliningStopLeavesServiceUntouched()
{
    FakeZapretBackend backend;
    backend.state = ZapretBackend::State::Active;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 100);

    controller.start();
    controller.confirmStop(false);

    QCOMPARE(backend.stopCalls, 0);
    QCOMPARE(backend.startCalls, 0);
    QCOMPARE(runner.starts, 0);
    QVERIFY(!controller.busy());
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Warning);
}

void BlockcheckControllerTest::cancelWhileStoppingRestoresServiceWithWarning()
{
    FakeZapretBackend backend;
    backend.state = ZapretBackend::State::Active;
    backend.autoStop = false;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 100);

    controller.start();
    controller.confirmStop(true);
    controller.cancel();
    QCOMPARE(runner.starts, 0);

    backend.completeStop(ZapretBackend::State::Inactive, true);
    QCOMPARE(backend.startCalls, 1);
    QVERIFY(!controller.busy());
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Warning);
    QVERIFY(controller.status().contains(QStringLiteral("Подбор отменён")));
}

void BlockcheckControllerTest::scanFailureRemainsErrorAfterServiceRestore()
{
    FakeZapretBackend backend;
    backend.state = ZapretBackend::State::Active;
    FakeBlockcheckRunner runner;
    runner.exitCode = 7;
    BlockcheckController controller(&backend, &runner, 100);

    controller.start();
    controller.confirmStop(true);

    QCOMPARE(backend.startCalls, 1);
    QVERIFY(!controller.busy());
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Error);
    QVERIFY(controller.status().contains(QStringLiteral("код 7")));
    QVERIFY(controller.status().contains(QStringLiteral("Служба восстановлена")));
}

void BlockcheckControllerTest::scanCancellationRemainsWarningAfterServiceRestore()
{
    FakeZapretBackend backend;
    backend.state = ZapretBackend::State::Active;
    FakeBlockcheckRunner runner;
    runner.autoFinish = false;
    BlockcheckController controller(&backend, &runner, 100);

    controller.start();
    controller.confirmStop(true);
    controller.cancel();

    QCOMPARE(runner.cancels, 1);
    QCOMPARE(backend.startCalls, 1);
    QVERIFY(!controller.busy());
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Warning);
}

void BlockcheckControllerTest::stopTimeoutWaitsForLateResultBeforeRestoring()
{
    FakeZapretBackend backend;
    backend.state = ZapretBackend::State::Active;
    backend.autoStop = false;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 10);

    controller.start();
    controller.confirmStop(true);
    QTRY_COMPARE_WITH_TIMEOUT(controller.statusType(), BlockcheckController::StatusType::Warning, 1000);
    QVERIFY(controller.busy());
    QCOMPARE(backend.startCalls, 0);

    backend.completeStop(ZapretBackend::State::Inactive, true);
    QCOMPARE(backend.startCalls, 1);
    QVERIFY(!controller.busy());
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Warning);
}

void BlockcheckControllerTest::stopFailureAttemptsRecoveryAndWarns()
{
    FakeZapretBackend backend;
    backend.state = ZapretBackend::State::Active;
    backend.autoStop = false;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 100);

    controller.start();
    controller.confirmStop(true);
    backend.completeStop(ZapretBackend::State::Inactive, false);

    QCOMPARE(backend.startCalls, 1);
    QVERIFY(!controller.busy());
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Warning);
    QVERIFY(controller.status().contains(QStringLiteral("Не удалось подтвердить остановку")));
}

void BlockcheckControllerTest::restoreFailureReportsError()
{
    FakeZapretBackend backend;
    backend.state = ZapretBackend::State::Active;
    backend.autoStart = false;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 100);

    controller.start();
    controller.confirmStop(true);
    backend.completeStart(ZapretBackend::State::Unknown, false);

    QVERIFY(!controller.busy());
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Error);
    QVERIFY(controller.status().contains(QStringLiteral("Не удалось восстановить службу")));
}

void BlockcheckControllerTest::verificationTimeoutFinishesSafely()
{
    FakeZapretBackend backend;
    backend.autoVerify = false;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 10);

    controller.start();
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 1000);
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Error);
    QCOMPARE(runner.starts, 0);

    emit backend.stateVerifiedForRequest(backend.lastRequestId, ZapretBackend::State::Active);
    QCOMPARE(runner.starts, 0);
    QVERIFY(!controller.busy());
}

void BlockcheckControllerTest::restoreTimeoutReportsError()
{
    FakeZapretBackend backend;
    backend.state = ZapretBackend::State::Active;
    backend.autoStart = false;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 10);

    controller.start();
    controller.confirmStop(true);
    QCOMPARE(backend.startCalls, 1);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 1000);
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Error);
    QVERIFY(controller.status().contains(QStringLiteral("Не удалось подтвердить восстановление")));
}


void BlockcheckControllerTest::ignoresLateVerificationFromPreviousOperation()
{
    FakeZapretBackend backend;
    backend.autoVerify = false;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 10);

    controller.start();
    const quint64 firstRequest = backend.lastRequestId;
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 1000);

    backend.autoVerify = true;
    backend.state = ZapretBackend::State::Inactive;
    controller.start();
    QCOMPARE(runner.starts, 1);

    backend.state = ZapretBackend::State::Active;
    emit backend.stateVerifiedForRequest(firstRequest, ZapretBackend::State::Active);
    QCOMPARE(runner.starts, 1);
    QVERIFY(!controller.busy());
}

void BlockcheckControllerTest::ignoresLateRestoreResultFromPreviousOperation()
{
    FakeZapretBackend backend;
    backend.state = ZapretBackend::State::Active;
    backend.autoStart = false;
    FakeBlockcheckRunner runner;
    BlockcheckController controller(&backend, &runner, 10);

    controller.start();
    controller.confirmStop(true);
    const quint64 oldRequest = backend.lastRequestId;
    backend.completeStart(oldRequest, ZapretBackend::State::Unknown, false);
    QTRY_VERIFY_WITH_TIMEOUT(!controller.busy(), 1000);

    backend.autoVerify = true;
    backend.state = ZapretBackend::State::Inactive;
    controller.start();
    QCOMPARE(runner.starts, 1);
    emit backend.unitActionFinishedForRequest(oldRequest, QStringLiteral("start"),
                                               ZapretBackend::State::Active, true);
    QVERIFY(!controller.busy());
    QCOMPARE(controller.statusType(), BlockcheckController::StatusType::Success);
    QCOMPARE(runner.starts, 1);
}

QTEST_GUILESS_MAIN(BlockcheckControllerTest)
#include "test_blockcheckcontroller.moc"
