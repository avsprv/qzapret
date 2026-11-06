#include <QDir>
#include <QFile>
#include <QTemporaryDir>
#include <QtTest>

#include "backend/updatechecker.h"

class TestUpdate : public QObject
{
    Q_OBJECT

    static void writeChanges(const QString &dir, const QByteArray &content)
    {
        QVERIFY(QDir().mkpath(dir + QStringLiteral("/docs")));
        QFile f(dir + QStringLiteral("/docs/changes.txt"));
        QVERIFY(f.open(QIODevice::WriteOnly));
        f.write(content);
    }

private slots:
    void normalizeStripsV()
    {
        QCOMPARE(ZapretVersion::normalize(QStringLiteral("v1.0.7")), QStringLiteral("1.0.7"));
        QCOMPARE(ZapretVersion::normalize(QStringLiteral(" 1.0.7 ")), QStringLiteral("1.0.7"));
    }

    void compareIsNumeric_data()
    {
        QTest::addColumn<QString>("a");
        QTest::addColumn<QString>("b");
        QTest::addColumn<int>("sign");
        QTest::newRow("newer patch") << "1.0.6" << "v1.0.7" << -1;
        QTest::newRow("equal") << "1.0.7" << "v1.0.7" << 0;
        QTest::newRow("older latest") << "1.0.7" << "1.0.6" << 1;
        QTest::newRow("numeric not lexical") << "1.0.9" << "1.0.10" << -1;
        QTest::newRow("four components") << "1.0.5.2" << "1.0.6" << -1;
        QTest::newRow("missing part is zero") << "1.0" << "1.0.0" << 0;
        QTest::newRow("major wins") << "2.0" << "1.9.9" << 1;
    }
    void compareIsNumeric()
    {
        QFETCH(QString, a);
        QFETCH(QString, b);
        QFETCH(int, sign);
        const int r = ZapretVersion::compare(a, b);
        QCOMPARE(r < 0 ? -1 : (r > 0 ? 1 : 0), sign);
    }

    void installedTakesLastHeading()
    {
        QTemporaryDir d;
        QVERIFY(d.isValid());
        writeChanges(d.path(),
                     "1.0.5.2\n\n* something 2.0 in text\n\n1.0.6  \n\n* x\n\n1.0.7\n\n* zapret-obfs: y\n");
        QCOMPARE(ZapretVersion::installed(d.path()), QStringLiteral("1.0.7"));
    }

    void installedEmptyWhenMissingOrNoHeading()
    {
        QTemporaryDir d;
        QVERIFY(d.isValid());
        QCOMPARE(ZapretVersion::installed(d.path()), QString());
        writeChanges(d.path(), "no versions here\n* just text 1.2.3\n");
        QCOMPARE(ZapretVersion::installed(d.path()), QString());
    }

    void tagFromLocationAcceptsOnlyUpstreamRelease_data()
    {
        QTest::addColumn<QString>("location");
        QTest::addColumn<QString>("tag");
        const QString ok = QStringLiteral("https://github.com/bol-van/zapret2/releases/tag/");
        QTest::newRow("normal") << ok + "v1.0.7" << "v1.0.7";
        QTest::newRow("four components") << ok + "v1.0.5.2" << "v1.0.5.2";
        QTest::newRow("other repo") << "https://github.com/evil/zapret2/releases/tag/v1.0.7" << "";
        QTest::newRow("other host") << "https://example.com/bol-van/zapret2/releases/tag/v1.0.7" << "";
        QTest::newRow("plain http") << "http://github.com/bol-van/zapret2/releases/tag/v1.0.7" << "";
        QTest::newRow("no tag") << "https://github.com/bol-van/zapret2/releases" << "";
        QTest::newRow("shell metachar") << ok + "v1.0.7;id" << "";
        QTest::newRow("trailing path") << ok + "v1.0.7/../x" << "";
        QTest::newRow("prefix attack") << "x" + ok + "v1.0.7" << "";
        QTest::newRow("empty") << "" << "";
    }
    void tagFromLocationAcceptsOnlyUpstreamRelease()
    {
        QFETCH(QString, location);
        QFETCH(QString, tag);
        QCOMPARE(ZapretVersion::tagFromLocation(location), tag);
    }
};

QTEST_GUILESS_MAIN(TestUpdate)
#include "test_update.moc"
