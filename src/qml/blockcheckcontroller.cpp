#include "blockcheckcontroller.h"

#include <QRegularExpression>
#include <QTimer>
#include "backend/systemdbackend.h"

BlockcheckController::BlockcheckController(QObject *parent)
    : QObject(parent), m_backend(new SystemdBackend(this)), m_runner(new BlockcheckRunner(this))
{
    connect(m_backend, &ZapretBackend::stateVerified, this, &BlockcheckController::handleVerifiedState);
    connect(m_backend, &ZapretBackend::unitActionFinished, this, &BlockcheckController::handleUnitAction);
    connect(m_backend, &ZapretBackend::errorOccurred, this, &BlockcheckController::appendLog);
    connect(m_runner, &BlockcheckRunner::logLine, this, &BlockcheckController::appendLog);
    connect(m_runner, &BlockcheckRunner::strategyFound, this, [this](const BlockcheckRunner::Result &r) {
        ++m_found;
        m_results.append(QVariantMap{{"label", QStringLiteral("[ipv%1 %2 %3] %4").arg(r.ipv, r.domain, r.test, r.commandLine())},
                                      {"command", r.commandLine()}});
        emit resultsChanged();
    });
    connect(m_runner, &BlockcheckRunner::finished, this, &BlockcheckController::handleFinished);
}
QString BlockcheckController::domains() const { return m_domains; }
void BlockcheckController::setDomains(const QString &v) { if (m_domains != v) { m_domains=v; emit optionsChanged(); } }
QString BlockcheckController::ipVersion() const { return m_ipVersion; }
void BlockcheckController::setIpVersion(const QString &v) { if (m_ipVersion != v) { m_ipVersion=v; emit optionsChanged(); } }
bool BlockcheckController::http() const { return m_http; }
void BlockcheckController::setHttp(bool v) { if (m_http != v) { m_http=v; emit optionsChanged(); } }
bool BlockcheckController::tls12() const { return m_tls12; }
void BlockcheckController::setTls12(bool v) { if (m_tls12 != v) { m_tls12=v; emit optionsChanged(); } }
bool BlockcheckController::tls13() const { return m_tls13; }
void BlockcheckController::setTls13(bool v) { if (m_tls13 != v) { m_tls13=v; emit optionsChanged(); } }
bool BlockcheckController::http3() const { return m_http3; }
void BlockcheckController::setHttp3(bool v) { if (m_http3 != v) { m_http3=v; emit optionsChanged(); } }
int BlockcheckController::repeats() const { return m_repeats; }
void BlockcheckController::setRepeats(int v) { v=qBound(1,v,10); if (m_repeats != v) { m_repeats=v; emit optionsChanged(); } }
QString BlockcheckController::scanLevel() const { return m_scanLevel; }
void BlockcheckController::setScanLevel(const QString &v) { if (m_scanLevel != v) { m_scanLevel=v; emit optionsChanged(); } }
QString BlockcheckController::logText() const { return m_logText; }
QVariantList BlockcheckController::results() const { return m_results; }
QString BlockcheckController::status() const { return m_status; }
bool BlockcheckController::busy() const { return m_busy; }
bool BlockcheckController::updateBusy() const { return m_updateBusy; }
void BlockcheckController::setUpdateBusy(bool v) { if (m_updateBusy != v) { m_updateBusy=v; emit updateBusyChanged(); } }
void BlockcheckController::appendLog(const QString &line) { m_logText += (m_logText.isEmpty() ? QString() : QStringLiteral("\n")) + line; emit logTextChanged(); }
void BlockcheckController::setStatus(const QString &v) { if (m_status != v) { m_status=v; emit statusChanged(); } }
void BlockcheckController::setBusy(bool v) { if (m_busy != v) { m_busy=v; emit busyChanged(); } }
BlockcheckRunner::Options BlockcheckController::collectOptions() const {
    BlockcheckRunner::Options o;
    o.domains=m_domains.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    o.ipvs=m_ipVersion; o.http=m_http; o.tls12=m_tls12; o.tls13=m_tls13; o.http3=m_http3;
    o.repeats=m_repeats; o.scanLevel=m_scanLevel; return o;
}
void BlockcheckController::start() {
    if (m_busy || m_updateBusy) { setStatus(tr("Подбор недоступен во время другой операции.")); return; }
    const auto o=collectOptions();
    static const QRegularExpression domainRe(QStringLiteral(R"(^[A-Za-z0-9](?:[A-Za-z0-9.-]{0,251}[A-Za-z0-9])?\.?$)"));
    if (o.domains.isEmpty() || o.domains.size()>10) { setStatus(tr("Укажите от 1 до 10 доменов.")); return; }
    for (const auto &d:o.domains) if (!domainRe.match(d).hasMatch() || d.contains(QStringLiteral(".."))) {
        setStatus(tr("Некорректный домен: %1").arg(d)); return;
    }
    if (o.ipvs!=QLatin1String("4") && o.ipvs!=QLatin1String("6") && o.ipvs!=QLatin1String("46")) { setStatus(tr("Недопустимый протокол IP.")); return; }
    if (!o.http && !o.tls12 && !o.tls13 && !o.http3) { setStatus(tr("Выберите хотя бы один тип проверки.")); return; }
    if (o.scanLevel!=QLatin1String("veryquick") && o.scanLevel!=QLatin1String("quick") && o.scanLevel!=QLatin1String("standard") && o.scanLevel!=QLatin1String("force")) { setStatus(tr("Недопустимый уровень сканирования.")); return; }
    m_lastOptions=o; m_found=0; m_results.clear(); emit resultsChanged(); m_logText.clear(); emit logTextChanged();
    m_resumeService=false; m_cancelRequested=false; m_phase=Phase::Checking; setBusy(true);
    setStatus(tr("Проверка состояния службы zapret2…")); m_backend->verifyState();
}
void BlockcheckController::handleVerifiedState(ZapretBackend::State state) {
    if (m_phase!=Phase::Checking) return;
    if (state==ZapretBackend::State::Inactive) { beginScan(); return; }
    if (state==ZapretBackend::State::Active) {
        m_phase=Phase::AwaitingConfirmation;
        emit confirmationRequired(tr("Служба zapret2 запущена"),
            tr("Для диагностики требуется временно остановить zapret2. После завершения приложение попытается восстановить работу службы. Продолжить?"));
        return;
    }
    finish(tr("Не удалось достоверно определить состояние службы. Диагностика не запущена."));
}
void BlockcheckController::confirmStop(bool accepted) {
    if (m_phase!=Phase::AwaitingConfirmation) return;
    if (!accepted) { finish(tr("Подбор отменён. Служба не изменена.")); return; }
    m_resumeService=true; m_phase=Phase::WaitingForStop; setStatus(tr("Остановка службы zapret2…")); m_backend->stop();
}
void BlockcheckController::beginScan() { m_phase=Phase::Scanning; setStatus(tr("Выполняется диагностика blockcheck2…")); appendLog(tr("Запуск blockcheck2…")); m_runner->start(m_lastOptions); }
void BlockcheckController::cancel() {
    if (m_phase==Phase::Checking || m_phase==Phase::AwaitingConfirmation) { finish(tr("Диагностика отменена до запуска.")); return; }
    if (m_phase==Phase::WaitingForStop) { m_cancelRequested=true; setStatus(tr("Отмена запрошена; ожидается подтверждение остановки службы.")); return; }
    if (m_phase==Phase::Scanning) { m_cancelRequested=true; m_runner->cancel(); }
}
void BlockcheckController::handleUnitAction(const QString &action, ZapretBackend::State state, bool success) {
    if (m_phase==Phase::WaitingForStop && action==QLatin1String("stop")) {
        if (success && state==ZapretBackend::State::Inactive) {
            if (m_cancelRequested) restoreService(tr("Диагностика отменена до запуска."));
            else beginScan();
        } else if (state==ZapretBackend::State::Active) { m_resumeService=false; finish(tr("Остановка не подтверждена. Диагностика не запущена.")); }
        else restoreService(tr("Не удалось подтвердить остановку службы."));
    } else if (m_phase==Phase::WaitingForStart && action==QLatin1String("start")) {
        m_resumeService=false;
        finish(state==ZapretBackend::State::Active ? tr("Служба zapret2 снова активна.") : tr("Не удалось восстановить службу. Проверьте состояние systemd."));
    }
}
void BlockcheckController::restoreService(const QString &reason) {
    if (!reason.isEmpty()) appendLog(reason);
    if (!m_resumeService) { finish(); return; }
    m_phase=Phase::WaitingForStart; setStatus(tr("Восстановление службы zapret2…")); m_backend->start();
}
void BlockcheckController::handleFinished(int exitCode, bool canceled) {
    m_cancelRequested=canceled;
    appendLog(canceled ? tr("Подбор отменён.") : tr("Диагностика завершена с кодом %1.").arg(exitCode));
    appendLog(tr("Найдено стратегий: %1; ошибок: %2").arg(m_found).arg(m_runner->errorCount()));
    if (m_resumeService) restoreService(tr("Восстановление исходного состояния службы…"));
    else finish();
}
void BlockcheckController::finish(const QString &message) {
    m_phase=Phase::Idle; m_resumeService=false;
    if (!message.isEmpty()) appendLog(message);
    setStatus(message.isEmpty() ? tr("Готово.") : message);
    setBusy(false);
    emit scanFinished(m_found,m_cancelRequested);
    m_cancelRequested=false;
}
