#include "blockcheckcontroller.h"

#include <QRegularExpression>
#include <QGuiApplication>
#include <QClipboard>
#include <QTimer>
#include <QSaveFile>
#include <QFileInfo>
#include "backend/systemdbackend.h"

BlockcheckController::BlockcheckController(QObject *parent)
    : BlockcheckController(nullptr, nullptr, 30000, parent)
{
}

BlockcheckController::BlockcheckController(ZapretBackend *backend, BlockcheckRunner *runner,
                                           int lifecycleTimeoutMs, QObject *parent)
    : QObject(parent), m_lifecycleTimeout(new QTimer(this)),
      m_backend(backend ? backend : new SystemdBackend(this)),
      m_runner(runner ? runner : new BlockcheckRunner(this))
{
    m_lifecycleTimeout->setSingleShot(true);
    m_lifecycleTimeout->setInterval(qMax(1, lifecycleTimeoutMs));
    connect(m_lifecycleTimeout, &QTimer::timeout, this, [this] {
        if (m_phase == Phase::Checking) finish(tr("Не удалось получить подтверждённое состояние службы. Диагностика не запущена."), StatusType::Error);
        else if (m_phase == Phase::WaitingForStop) {
            m_stopTimedOut = true;
            appendLog(tr("Операция остановки выполняется дольше ожидаемого. Новая команда запуска не отправляется до получения результата остановки."));
            setStatus(tr("Ожидание завершения операции остановки службы…"), StatusType::Warning);
        }
        else if (m_phase == Phase::WaitingForStart) { m_resumeService=false; finish(tr("Не удалось подтвердить восстановление службы. Проверьте systemd."), StatusType::Error); }
    });
    connect(m_backend, &ZapretBackend::stateVerifiedForRequest, this, &BlockcheckController::handleVerifiedState);
    connect(m_backend, &ZapretBackend::unitActionFinishedForRequest, this, &BlockcheckController::handleUnitAction);
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
BlockcheckController::StatusType BlockcheckController::statusType() const { return m_statusType; }
QString BlockcheckController::statusSeverity() const {
    switch (m_statusType) {
    case StatusType::Success: return QStringLiteral("Success");
    case StatusType::Warning: return QStringLiteral("Warning");
    case StatusType::Error: return QStringLiteral("Error");
    case StatusType::Information: return QStringLiteral("Information");
    }
    return QStringLiteral("Information");
}
bool BlockcheckController::busy() const { return m_busy; }
bool BlockcheckController::updateBusy() const { return m_updateBusy; }
void BlockcheckController::setUpdateBusy(bool v) { if (m_updateBusy != v) { m_updateBusy=v; emit updateBusyChanged(); } }
void BlockcheckController::appendLog(const QString &line) {
    constexpr qsizetype maxLogChars = 1024 * 1024;
    QString boundedLine = line;
    if (boundedLine.size() > 64 * 1024)
        boundedLine = boundedLine.left(64 * 1024) + QStringLiteral("… [строка журнала сокращена]");
    m_logText += (m_logText.isEmpty() ? QString() : QStringLiteral("\n")) + boundedLine;
    if (m_logText.size() > maxLogChars)
        m_logText.remove(0, m_logText.size() - maxLogChars);
    emit logTextChanged();
}
void BlockcheckController::setStatus(const QString &v, StatusType type) {
    if (m_status != v || m_statusType != type) {
        m_status = v;
        m_statusType = type;
        emit statusChanged();
    }
}
void BlockcheckController::setBusy(bool v) { if (m_busy != v) { m_busy=v; emit busyChanged(); } }
BlockcheckRunner::Options BlockcheckController::collectOptions() const {
    BlockcheckRunner::Options o;
    o.domains=m_domains.simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    o.ipvs=m_ipVersion; o.http=m_http; o.tls12=m_tls12; o.tls13=m_tls13; o.http3=m_http3;
    o.repeats=m_repeats; o.scanLevel=m_scanLevel; return o;
}
void BlockcheckController::start() {
    if (m_busy || m_updateBusy) { setStatus(tr("Подбор недоступен во время другой операции."), StatusType::Warning); return; }
    const auto o=collectOptions();
    static const QRegularExpression labelRe(QStringLiteral("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?$"));
    if (o.domains.isEmpty() || o.domains.size()>10) { setStatus(tr("Укажите от 1 до 10 доменов."), StatusType::Error); return; }
    for (const auto &input : o.domains) {
        QString domain = input;
        if (domain.endsWith(QLatin1Char('.'))) domain.chop(1);
        if (domain.isEmpty() || domain.size()>253) {
            setStatus(tr("Некорректный домен: %1").arg(input), StatusType::Error); return;
        }
        bool valid = true;
        for (const auto &label : domain.split(QLatin1Char('.')))
            valid = valid && label.size()<=63 && labelRe.match(label).hasMatch();
        if (!valid) { setStatus(tr("Некорректный домен: %1").arg(input), StatusType::Error); return; }
    }
    if (o.ipvs!=QLatin1String("4") && o.ipvs!=QLatin1String("6") && o.ipvs!=QLatin1String("46")) { setStatus(tr("Недопустимый протокол IP."), StatusType::Error); return; }
    if (!o.http && !o.tls12 && !o.tls13 && !o.http3) { setStatus(tr("Выберите хотя бы один тип проверки."), StatusType::Error); return; }
    if (o.scanLevel!=QLatin1String("veryquick") && o.scanLevel!=QLatin1String("quick") && o.scanLevel!=QLatin1String("standard") && o.scanLevel!=QLatin1String("force")) { setStatus(tr("Недопустимый уровень сканирования."), StatusType::Error); return; }
    m_lastOptions=o; m_found=0; m_results.clear(); emit resultsChanged(); m_logText.clear(); emit logTextChanged();
    m_resumeService=false; m_cancelRequested=false; m_stopTimedOut=false; m_phase=Phase::Checking; ++m_operationId; setBusy(true);
    setStatus(tr("Проверка состояния службы zapret2…")); m_lifecycleTimeout->start(); m_backend->verifyState(m_operationId);
}
void BlockcheckController::handleVerifiedState(quint64 requestId, ZapretBackend::State state) {
    if (requestId != m_operationId || m_phase!=Phase::Checking) return;
    m_lifecycleTimeout->stop();
    if (state==ZapretBackend::State::Inactive) { beginScan(); return; }
    if (state==ZapretBackend::State::Active) {
        m_phase=Phase::AwaitingConfirmation;
        emit confirmationRequired(tr("Служба zapret2 запущена"),
            tr("Для диагностики требуется временно остановить zapret2. После завершения приложение попытается восстановить работу службы. Продолжить?"));
        return;
    }
    finish(tr("Не удалось достоверно определить состояние службы. Диагностика не запущена."), StatusType::Error);
}
void BlockcheckController::confirmStop(bool accepted) {
    if (m_phase!=Phase::AwaitingConfirmation) return;
    if (!accepted) { finish(tr("Подбор отменён. Служба не изменена."), StatusType::Warning); return; }
    m_resumeService=true; m_phase=Phase::WaitingForStop; setStatus(tr("Остановка службы zapret2…")); m_lifecycleTimeout->start(); m_backend->stop(m_operationId);
}
void BlockcheckController::beginScan() { m_phase=Phase::Scanning; setStatus(tr("Выполняется диагностика blockcheck2…")); appendLog(tr("Запуск blockcheck2…")); m_runner->start(m_lastOptions); }
bool BlockcheckController::saveLog(const QUrl &url) {
    if (!url.isLocalFile() || url.toLocalFile().isEmpty()) {
        setStatus(tr("Выберите локальный файл для сохранения журнала."), StatusType::Error);
        return false;
    }
    const QString path = url.toLocalFile();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly | QIODevice::Text)) {
        setStatus(tr("Не удалось открыть файл журнала: %1").arg(file.errorString()), StatusType::Error);
        return false;
    }
    const QByteArray data = m_logText.toUtf8();
    if (file.write(data) != data.size() || !file.commit()) {
        setStatus(tr("Не удалось сохранить журнал: %1").arg(file.errorString()), StatusType::Error);
        return false;
    }
    setStatus(tr("Журнал сохранён: %1").arg(QFileInfo(path).fileName()), StatusType::Success);
    return true;
}
void BlockcheckController::copyStrategy(int index) {
    if (index < 0 || index >= m_results.size()) return;
    const QString command = m_results.at(index).toMap().value(QStringLiteral("command")).toString();
    if (!command.isEmpty() && QGuiApplication::clipboard()) QGuiApplication::clipboard()->setText(command);
}
void BlockcheckController::cancel() {
    if (m_phase==Phase::Checking || m_phase==Phase::AwaitingConfirmation) { finish(tr("Диагностика отменена до запуска."), StatusType::Warning); return; }
    if (m_phase==Phase::WaitingForStop) { m_cancelRequested=true; setStatus(tr("Отмена запрошена; ожидается подтверждение остановки службы."), StatusType::Warning); return; }
    if (m_phase==Phase::Scanning) { m_cancelRequested=true; m_runner->cancel(); }
}
void BlockcheckController::handleUnitAction(quint64 requestId, const QString &action, ZapretBackend::State state, bool success) {
    if (requestId != m_operationId) return;
    if (m_phase==Phase::WaitingForStop && action==QLatin1String("stop")) {
        m_lifecycleTimeout->stop();
        if (m_stopTimedOut) {
            m_stopTimedOut = false;
            if (state == ZapretBackend::State::Active) {
                m_resumeService = false;
                finish(tr("Операция остановки завершилась с активной службой. Диагностика не запускалась."), StatusType::Error);
            } else {
                restoreService(tr("Получен результат операции остановки после тайм-аута."),
                               tr("Остановка службы заняла слишком много времени; диагностика не запускалась."),
                               StatusType::Warning);
            }
            return;
        }
        if (success && state==ZapretBackend::State::Inactive) {
            if (m_cancelRequested) {
                restoreService(tr("Диагностика отменена до запуска."),
                               tr("Подбор отменён. Служба восстановлена."), StatusType::Warning);
            } else {
                beginScan();
            }
        } else if (state==ZapretBackend::State::Active) {
            m_resumeService=false;
            finish(tr("Остановка не подтверждена. Диагностика не запущена."), StatusType::Error);
        } else {
            restoreService(tr("Не удалось подтвердить остановку службы."),
                           tr("Не удалось подтвердить остановку; выполнена попытка восстановления службы."),
                           StatusType::Warning);
        }
    } else if (m_phase==Phase::WaitingForStart && action==QLatin1String("start")) {
        m_lifecycleTimeout->stop();
        m_resumeService=false;
        if (state == ZapretBackend::State::Active) {
            const QString message = m_pendingFinalMessage.isEmpty()
                ? tr("Служба zapret2 снова активна.") : m_pendingFinalMessage;
            finish(message, m_pendingFinalType);
        } else {
            finish(tr("Не удалось восстановить службу. Проверьте состояние systemd."), StatusType::Error);
        }
    }
}
void BlockcheckController::restoreService(const QString &reason, const QString &finalMessage,
                                          StatusType finalType) {
    if (!reason.isEmpty()) appendLog(reason);
    if (!finalMessage.isEmpty()) m_pendingFinalMessage = finalMessage;
    m_pendingFinalType = finalType;
    if (!m_resumeService) {
        finish(m_pendingFinalMessage, m_pendingFinalType);
        return;
    }
    m_phase=Phase::WaitingForStart; setStatus(tr("Восстановление службы zapret2…")); m_lifecycleTimeout->start(); m_backend->start(m_operationId);
}
void BlockcheckController::handleFinished(int exitCode, bool canceled) {
    m_cancelRequested=canceled;
    appendLog(canceled ? tr("Подбор отменён.") : tr("Диагностика завершена с кодом %1.").arg(exitCode));
    appendLog(tr("Найдено стратегий: %1; ошибок: %2").arg(m_found).arg(m_runner->errorCount()));
    if (m_resumeService) {
        const StatusType finalType = canceled ? StatusType::Warning
            : (exitCode != 0 ? StatusType::Error : StatusType::Success);
        const QString finalMessage = canceled ? tr("Подбор отменён. Служба восстановлена.")
            : (exitCode != 0
                ? tr("Диагностика завершилась с ошибкой (код %1). Служба восстановлена.").arg(exitCode)
                : tr("Диагностика завершена. Служба восстановлена."));
        restoreService(tr("Восстановление исходного состояния службы…"), finalMessage, finalType);
    } else if (!canceled && exitCode != 0) {
        finish(tr("Диагностика завершилась с ошибкой (код %1).").arg(exitCode), StatusType::Error);
    } else if (canceled) {
        finish(tr("Подбор отменён."), StatusType::Warning);
    } else {
        finish(tr("Диагностика завершена."), StatusType::Success);
    }
}
void BlockcheckController::finish(const QString &message, StatusType type) {
    m_lifecycleTimeout->stop();
    m_phase=Phase::Idle; m_resumeService=false; m_stopTimedOut=false;
    if (!message.isEmpty()) appendLog(message);
    setStatus(message.isEmpty() ? tr("Готово.") : message, type);
    setBusy(false);
    emit scanFinished(m_found,m_cancelRequested);
    m_cancelRequested=false;
    m_pendingFinalMessage.clear();
    m_pendingFinalType=StatusType::Success;
}
