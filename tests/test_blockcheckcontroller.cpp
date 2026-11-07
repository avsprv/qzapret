#include <QtTest>

#include "blockcheckcontroller.h"

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

QTEST_GUILESS_MAIN(BlockcheckControllerTest)
#include "test_blockcheckcontroller.moc"
