#include <QtTest>
#include "profileanalyzer.h"
class TestProfileAnalyzer : public QObject {
    Q_OBJECT
private slots:
    void parsesUpstreamStyleProfiles() {
        const QString config = QStringLiteral(
            "NFQWS2_ENABLE=1\n"
            "NFQWS2_OPT=\"\n"
            "--filter-tcp=80 --filter-l7=http <HOSTLIST> --payload=http_req "
            "--lua-desync=fake:blob=fake_default_http --new\n"
            "--filter-tcp=443 --filter-l7=tls --payload=tls_client_hello "
            "--lua-desync=multisplit:pos=method+2\n"
            "\"\n");
        const auto result = ProfileAnalyzer::parse(config);
        QVERIFY2(result.ok(), qPrintable(result.error));
        QCOMPARE(result.profiles.size(), 2);
        QCOMPARE(result.profiles.at(0).filters.size(), 2);
        QCOMPARE(result.profiles.at(0).payloads.size(), 1);
        QCOMPARE(result.profiles.at(0).luaDesync.size(), 1);
        QCOMPARE(result.profiles.at(1).allArguments.size(), 4);
    }
    void parsesSingleProfileWithoutNewDelimiter() {
        const auto result = ProfileAnalyzer::parse(
            QStringLiteral("NFQWS2_OPT=\"--filter-udp=443 --payload=quic_initial\"\n"));
        QVERIFY(result.ok());
        QCOMPARE(result.profiles.size(), 1);
    }
    void rejectsMissingOrDuplicateAssignment() {
        QVERIFY(!ProfileAnalyzer::parse(QStringLiteral("NFQWS2_ENABLE=1\n")).ok());
        QVERIFY(!ProfileAnalyzer::parse(QStringLiteral(
            "NFQWS2_OPT=\"--filter-tcp=80\"\nNFQWS2_OPT=\"--filter-tcp=443\"\n")).ok());
    }
    void rejectsDynamicShellExpansion() {
        QVERIFY(!ProfileAnalyzer::parse(QStringLiteral("NFQWS2_OPT=\"--filter-tcp=$PORT\"\n")).ok());
        QVERIFY(!ProfileAnalyzer::parse(QStringLiteral("NFQWS2_OPT=\"--filter-tcp=$(id)\"\n")).ok());
        QVERIFY(!ProfileAnalyzer::parse(QStringLiteral("NFQWS2_OPT=\"--filter-tcp=80\\n\"\n")).ok());
    }
    void rejectsUnquotedAndUnterminatedValues() {
        QVERIFY(!ProfileAnalyzer::parse(QStringLiteral("NFQWS2_OPT=--filter-tcp=80\n")).ok());
        QVERIFY(!ProfileAnalyzer::parse(QStringLiteral("NFQWS2_OPT=\"--filter-tcp=80\n")).ok());
    }
    void rejectsEmptyProfiles() {
        QVERIFY(!ProfileAnalyzer::parse(QStringLiteral("NFQWS2_OPT=\"--new --filter-tcp=80\"\n")).ok());
        QVERIFY(!ProfileAnalyzer::parse(QStringLiteral("NFQWS2_OPT=\"--filter-tcp=80 --new\"\n")).ok());
        QVERIFY(!ProfileAnalyzer::parse(QStringLiteral("NFQWS2_OPT=\"--filter-tcp=80 --new --new --filter-tcp=443\"\n")).ok());
    }
    void ignoresCommentedAssignment() {
        const auto result = ProfileAnalyzer::parse(QStringLiteral(
            "# NFQWS2_OPT=\"--filter-tcp=80\"\n"
            "NFQWS2_OPT=\"--filter-tcp=443\"\n"));
        QVERIFY(result.ok());
        QCOMPARE(result.profiles.size(), 1);
    }
};
QTEST_GUILESS_MAIN(TestProfileAnalyzer)
#include "test_profileanalyzer.moc"
