#include <QApplication>
#include <QFile>
#include <QMessageBox>
#include <QPushButton>
#include <QSignalSpy>
#include <QTemporaryDir>
#include <QTimer>
#include <QtTest>

#include "blockcheckpage.h"

class FakeBackend final : public ZapretBackend
{
    Q_OBJECT
public:
    using ZapretBackend::ZapretBackend;

    State state = State::Inactive;
    int starts = 0;
    int stops = 0;

    void refresh() override { emit stateChanged(state); }
    void verifyState() override
    {
        emit stateChanged(state);
        emit stateVerified(state);
    }
    void start() override
    {
        ++starts;
        state = State::Active;
        emit stateChanged(state);
        emit unitActionFinished(QStringLiteral("start"), state, true);
    }
    void stop() override
    {
        ++stops;
        state = State::Inactive;
        emit stateChanged(state);
        emit unitActionFinished(QStringLiteral("stop"), state, true);
    }
    void setBootEnabled(bool) override {}
};

class TestBlockcheckLifecycle : public QObject
{
    Q_OBJECT
private:
    QString makeHelper(QTemporaryDir &dir)
    {
        const QString path = dir.filePath(QStringLiteral("fake-helper"));
        QFile file(path);
        if (!file.open(QIODevice::WriteOnly))
            return {};
        file.write("#!/bin/sh\n"
                   "echo 'fake blockcheck completed'\n"
                   "exit 0\n");
        file.close();
        file.setPermissions(file.permissions() | QFileDevice::ExeUser);
        return path;
    }

    QPushButton *startButton(BlockcheckPage &page)
    {
        for (auto *button : page.findChildren<QPushButton *>())
            if (button->text() == QStringLiteral("Начать подбор"))
                return button;
        return nullptr;
    }

private slots:
    void cleanup()
    {
        qunsetenv("QZAPRET_HELPER");
        qunsetenv("QZAPRET_NO_PKEXEC");
    }

    void inactiveServiceRunsScan()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString helper = makeHelper(dir);
        QVERIFY(!helper.isEmpty());
        qputenv("QZAPRET_HELPER", helper.toUtf8());
        qputenv("QZAPRET_NO_PKEXEC", "1");

        FakeBackend backend;
        backend.state = ZapretBackend::State::Inactive;
        BlockcheckPage page(&backend);
        QSignalSpy finished(&page, &BlockcheckPage::scanFinished);
        auto *button = startButton(page);
        QVERIFY(button);
        button->click();

        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 5000);
        QCOMPARE(backend.stops, 0);
        QCOMPARE(backend.starts, 0);
        QVERIFY(!page.isRunning());
    }

    void unknownServiceDoesNotRunScan()
    {
        FakeBackend backend;
        backend.state = ZapretBackend::State::Unknown;
        BlockcheckPage page(&backend);
        QSignalSpy finished(&page, &BlockcheckPage::scanFinished);
        auto *button = startButton(page);
        QVERIFY(button);
        button->click();

        QCOMPARE(finished.count(), 0);
        QCOMPARE(backend.stops, 0);
        QCOMPARE(backend.starts, 0);
        QVERIFY(!page.isRunning());
    }

    void activeServiceIsStoppedAndRestored()
    {
        QTemporaryDir dir;
        QVERIFY(dir.isValid());
        const QString helper = makeHelper(dir);
        QVERIFY(!helper.isEmpty());
        qputenv("QZAPRET_HELPER", helper.toUtf8());
        qputenv("QZAPRET_NO_PKEXEC", "1");

        FakeBackend backend;
        backend.state = ZapretBackend::State::Active;
        BlockcheckPage page(&backend);
        QSignalSpy finished(&page, &BlockcheckPage::scanFinished);
        QTimer::singleShot(0, [] {
            for (QWidget *widget : QApplication::topLevelWidgets()) {
                if (auto *box = qobject_cast<QMessageBox *>(widget)) {
                    if (auto *yes = box->button(QMessageBox::Yes))
                        yes->click();
                }
            }
        });
        auto *button = startButton(page);
        QVERIFY(button);
        button->click();

        QTRY_COMPARE_WITH_TIMEOUT(finished.count(), 1, 5000);
        QCOMPARE(backend.stops, 1);
        QCOMPARE(backend.starts, 1);
        QCOMPARE(backend.state, ZapretBackend::State::Active);
        QVERIFY(!page.isRunning());
    }
};

QTEST_MAIN(TestBlockcheckLifecycle)
#include "test_blockcheck_lifecycle.moc"
