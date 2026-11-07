#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "backend/blockcheckrunner.h"
#include "backend/zapretcompatibility.h"

class TestZapretCompatibility : public QObject
{
    Q_OBJECT
private:
    static void writeFile(const QString &path, const QByteArray &content)
    {
        QFile file(path);
        QVERIFY(file.open(QIODevice::WriteOnly));
        QCOMPARE(file.write(content), qint64(content.size()));
    }

    static void createCompatibleTree(const QString &root)
    {
        const QStringList directories{
            QStringLiteral("common"),
            QStringLiteral("blockcheck2.d/standard"),
            QStringLiteral("nfq2"),
            QStringLiteral("mdig"),
            QStringLiteral("docs")
        };
        for (const QString &directory : directories)
            QVERIFY(QDir().mkpath(root + QLatin1Char('/') + directory));

        const QByteArray script =
            "ENABLE_HTTP ENABLE_HTTPS_TLS12 ENABLE_HTTPS_TLS13 ENABLE_HTTP3 "
            "REPEATS SCANLEVEL working strategy found for ipv\n"
            "ask_list SCANLEVEL \"veryquick quick standard force\"\n";
        writeFile(root + QStringLiteral("/blockcheck2.sh"), script);
        writeFile(root + QStringLiteral("/config.default"), "# default config\n");
        writeFile(root + QStringLiteral("/common/base.sh"), "# common\n");
        writeFile(root + QStringLiteral("/common/dialog.sh"), "# dialog\n");
        writeFile(root + QStringLiteral("/blockcheck2.d/standard/10-http-basic.sh"), "# tests\n");
        writeFile(root + QStringLiteral("/nfq2/nfqws2"), "#!/bin/sh\n");
        writeFile(root + QStringLiteral("/docs/changes.txt"), "v1.0.6\n\n1.0.7\n\nrelease\n");
    }

private slots:
    void detectsCompatibleUpstream()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        createCompatibleTree(temp.path());

        const auto report = ZapretCompatibility::inspect(temp.path());
        QVERIFY2(report.compatible, qPrintable(report.missingComponents.join(QStringLiteral(", "))));
        QCOMPARE(report.version, QStringLiteral("1.0.7"));
        QVERIFY(report.missingComponents.isEmpty());
    }

    void reportsMissingComponents()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        createCompatibleTree(temp.path());
        QVERIFY(QFile::remove(temp.path() + QStringLiteral("/common/dialog.sh")));

        const auto report = ZapretCompatibility::inspect(temp.path());
        QVERIFY(!report.compatible);
        QVERIFY(report.missingComponents.contains(QStringLiteral("common/dialog.sh")));
    }

    void rejectsMissingCapabilities()
    {
        QTemporaryDir temp;
        QVERIFY(temp.isValid());
        createCompatibleTree(temp.path());
        writeFile(temp.path() + QStringLiteral("/blockcheck2.sh"),
                  "ENABLE_HTTP SCANLEVEL\nask_list SCANLEVEL \"quick standard\"\n");

        const auto report = ZapretCompatibility::inspect(temp.path());
        QVERIFY(!report.compatible);
        QVERIFY(report.missingComponents.contains(
            QStringLiteral("blockcheck2.sh: scan level veryquick")));
        QVERIFY(report.missingComponents.contains(
            QStringLiteral("blockcheck2.sh: scan level force")));
    }

    void parsesUpstreamOutputFixture()
    {
        QFile fixture(QStringLiteral(QZAPRET_SOURCE_DIR "/tests/fixtures/blockcheck2-1.0.7.log"));
        QVERIFY(fixture.open(QIODevice::ReadOnly | QIODevice::Text));
        const QStringList lines = QString::fromUtf8(fixture.readAll()).split(
            QLatin1Char('\n'), Qt::SkipEmptyParts);

        QList<BlockcheckRunner::Result> results;
        for (const QString &line : lines) {
            if (const auto result = BlockcheckRunner::parseLine(line))
                results.append(*result);
        }

        QCOMPARE(results.size(), 2);
        QCOMPARE(results.at(0).ipv, QStringLiteral("4"));
        QCOMPARE(results.at(1).ipv, QStringLiteral("6"));
        QCOMPARE(results.at(0).domain, QStringLiteral("rutracker.org"));
        QCOMPARE(results.at(1).domain, QStringLiteral("example.org"));
    }
};

QTEST_GUILESS_MAIN(TestZapretCompatibility)
#include "test_zapretcompatibility.moc"
