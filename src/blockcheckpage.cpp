#include "blockcheckpage.h"

#include <QCheckBox>
#include <QFontDatabase>
#include <QClipboard>
#include <QDateTime>
#include <QFileDialog>
#include <QComboBox>
#include <QFormLayout>
#include <QGuiApplication>
#include <QHBoxLayout>
#include <QLabel>
#include <QLineEdit>
#include <QListWidget>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSaveFile>
#include <QTextDocument>
#include <QSpinBox>
#include <QTimer>
#include <QVBoxLayout>

BlockcheckPage::BlockcheckPage(ZapretBackend *backend, QWidget *parent)
    : QWidget(parent)
    , m_backend(backend)
    , m_runner(new BlockcheckRunner(this))
    , m_domains(new QLineEdit(QStringLiteral("rutracker.org"), this))
    , m_ipvs(new QComboBox(this))
    , m_http(new QCheckBox(tr("HTTP"), this))
    , m_tls12(new QCheckBox(tr("HTTPS TLS 1.2"), this))
    , m_tls13(new QCheckBox(tr("HTTPS TLS 1.3"), this))
    , m_http3(new QCheckBox(tr("HTTP/3 (QUIC)"), this))
    , m_repeats(new QSpinBox(this))
    , m_scan(new QComboBox(this))
    , m_startBtn(new QPushButton(tr("Начать подбор"), this))
    , m_cancelBtn(new QPushButton(tr("Отмена"), this))
    , m_copyBtn(new QPushButton(tr("Копировать стратегию"), this))
    , m_saveLogBtn(new QPushButton(tr("Сохранить журнал…"), this))
    , m_log(new QPlainTextEdit(this))
    , m_results(new QListWidget(this))
    , m_stopTimeout(new QTimer(this))
{
    m_domains->setPlaceholderText(tr("домены через пробел (до 10)"));
    m_ipvs->addItem(QStringLiteral("IPv4"), QStringLiteral("4"));
    m_ipvs->addItem(QStringLiteral("IPv6"), QStringLiteral("6"));
    m_ipvs->addItem(QStringLiteral("IPv4 + IPv6"), QStringLiteral("46"));
    m_http->setChecked(true);
    m_tls12->setChecked(true);
    m_repeats->setRange(1, 10);
    for (const char *lvl : {"veryquick", "quick", "standard", "force"})
        m_scan->addItem(QLatin1String(lvl));
    m_scan->setCurrentText(QStringLiteral("standard"));
    m_scan->setToolTip(tr("veryquick/quick быстрее, но могут пропускать рабочие стратегии; "
                          "force проверяет всё."));
    m_log->setReadOnly(true);
    m_log->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    m_cancelBtn->setEnabled(false);
    m_copyBtn->setEnabled(false);
    m_saveLogBtn->setEnabled(false);

    auto *checks = new QHBoxLayout;
    for (auto *c : {m_http, m_tls12, m_tls13, m_http3})
        checks->addWidget(c);
    checks->addStretch();

    auto *form = new QFormLayout;
    form->addRow(tr("Домены:"), m_domains);
    form->addRow(tr("Протокол IP:"), m_ipvs);
    form->addRow(tr("Проверять:"), checks);
    form->addRow(tr("Повторов:"), m_repeats);
    form->addRow(tr("Уровень:"), m_scan);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_startBtn);
    buttons->addWidget(m_cancelBtn);
    buttons->addStretch();

    auto *note = new QLabel(tr("Результат — рекомендация blockcheck2, а не готовое решение: "
                               "стратегии стоит проверять вручную."),
                            this);
    note->setWordWrap(true);
    note->setEnabled(false);

    auto *layout = new QVBoxLayout(this);
    layout->addLayout(form);
    layout->addLayout(buttons);
    layout->addWidget(new QLabel(tr("Найденные стратегии:"), this));
    layout->addWidget(m_results, 1);
    layout->addWidget(m_copyBtn, 0, Qt::AlignLeft);
    auto *logHeader = new QHBoxLayout;
    logHeader->addWidget(new QLabel(tr("Журнал:"), this));
    logHeader->addStretch();
    logHeader->addWidget(m_saveLogBtn);
    layout->addLayout(logHeader);
    layout->addWidget(m_log, 2);
    layout->addWidget(note);

    m_stopTimeout->setSingleShot(true);
    m_stopTimeout->setInterval(60000);
    connect(m_stopTimeout, &QTimer::timeout, this, [this] {
        m_waitingStop = false;
        m_resumeService = false;
        setBusy(false);
        m_log->appendPlainText(tr("Не удалось остановить сервис zapret2, подбор не начат."));
    });

    connect(m_startBtn, &QPushButton::clicked, this, &BlockcheckPage::onStartClicked);
    connect(m_cancelBtn, &QPushButton::clicked, this, [this] {
        m_cancelBtn->setEnabled(false);
        if (m_waitingStop) {
            m_stopTimeout->stop();
            m_waitingStop = false;
            m_resumeService = false;
            setBusy(false);
        } else {
            m_runner->cancel();
        }
    });
    connect(m_saveLogBtn, &QPushButton::clicked, this, &BlockcheckPage::saveLog);
    connect(m_log, &QPlainTextEdit::textChanged, this, [this] {
        m_saveLogBtn->setEnabled(!m_log->document()->isEmpty());
    });
    connect(m_copyBtn, &QPushButton::clicked, this, [this] {
        if (auto *it = m_results->currentItem())
            QGuiApplication::clipboard()->setText(it->data(Qt::UserRole).toString());
    });
    connect(m_results, &QListWidget::currentItemChanged, this, [this] {
        m_copyBtn->setEnabled(m_results->currentItem() != nullptr);
    });
    connect(m_backend, &ZapretBackend::stateChanged, this, &BlockcheckPage::onBackendState);
    connect(m_runner, &BlockcheckRunner::logLine, m_log, &QPlainTextEdit::appendPlainText);
    connect(m_runner, &BlockcheckRunner::strategyFound, this, [this](const BlockcheckRunner::Result &r) {
        ++m_found;
        auto *item = new QListWidgetItem(
            QStringLiteral("[ipv%1 %2 %3] %4").arg(r.ipv, r.domain, r.test, r.commandLine()));
        item->setData(Qt::UserRole, r.commandLine());
        m_results->addItem(item);
    });
    connect(m_runner, &BlockcheckRunner::finished, this, &BlockcheckPage::onFinished);
}

BlockcheckRunner::Options BlockcheckPage::collectOptions() const
{
    BlockcheckRunner::Options o;
    o.domains = m_domains->text().simplified().split(QLatin1Char(' '), Qt::SkipEmptyParts);
    o.ipvs = m_ipvs->currentData().toString();
    o.http = m_http->isChecked();
    o.tls12 = m_tls12->isChecked();
    o.tls13 = m_tls13->isChecked();
    o.http3 = m_http3->isChecked();
    o.repeats = m_repeats->value();
    o.scanLevel = m_scan->currentText();
    return o;
}

void BlockcheckPage::setBusy(bool busy)
{
    for (QWidget *w : std::initializer_list<QWidget *>{m_domains, m_ipvs, m_http, m_tls12, m_tls13,
                                                       m_http3, m_repeats, m_scan, m_startBtn})
        w->setEnabled(!busy);
    m_cancelBtn->setEnabled(busy);
}

void BlockcheckPage::onStartClicked()
{
    const auto o = collectOptions();
    if (o.domains.isEmpty() || o.domains.size() > 10) {
        QMessageBox::warning(this, tr("Подбор стратегий"), tr("Укажите от 1 до 10 доменов."));
        return;
    }
    if (!o.http && !o.tls12 && !o.tls13 && !o.http3) {
        QMessageBox::warning(this, tr("Подбор стратегий"), tr("Выберите хотя бы один тип проверки."));
        return;
    }

    m_found = 0;
    m_results->clear();
    m_log->clear();

    if (m_state == ZapretBackend::State::Active) {
        const auto answer = QMessageBox::question(
            this, tr("Сервис zapret2 запущен"),
            tr("Подбор не работает при запущенном zapret2. Остановить сервис на время подбора "
               "и запустить снова после завершения?"));
        if (answer != QMessageBox::Yes)
            return;
        m_resumeService = true;
        m_waitingStop = true;
        setBusy(true);
        m_log->appendPlainText(tr("Останавливаю zapret2…"));
        m_stopTimeout->start();
        m_backend->stop();
        return;
    }
    beginScan();
}

void BlockcheckPage::onBackendState(ZapretBackend::State state)
{
    m_state = state;
    if (m_waitingStop && state != ZapretBackend::State::Active) {
        m_stopTimeout->stop();
        m_waitingStop = false;
        beginScan();
    }
}

void BlockcheckPage::beginScan()
{
    setBusy(true);
    m_log->appendPlainText(tr("Запускаю blockcheck2…"));
    m_runner->start(collectOptions());
}

void BlockcheckPage::saveLog()
{
    if (m_log->document()->isEmpty())
        return;

    const QString defaultName = QStringLiteral("blockcheck2-log-%1.txt")
                                    .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Сохранить журнал"), defaultName,
        tr("Текстовые файлы (*.txt);;Все файлы (*)"));
    if (path.isEmpty())
        return;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly)) {
        QMessageBox::critical(this, tr("Ошибка сохранения"),
                              tr("Не удалось открыть файл для записи: %1").arg(file.errorString()));
        return;
    }

    const QByteArray data = m_log->toPlainText().toUtf8();
    if (file.write(data) != data.size() || !file.commit()) {
        QMessageBox::critical(this, tr("Ошибка сохранения"),
                              tr("Не удалось сохранить журнал: %1").arg(file.errorString()));
    }
}

void BlockcheckPage::onFinished(int exitCode, bool canceled)
{
    setBusy(false);
    if (canceled)
        m_log->appendPlainText(tr("Подбор отменён."));
    else if (exitCode != 0)
        m_log->appendPlainText(tr("blockcheck2 завершился с кодом %1.").arg(exitCode));
    if (m_resumeService) {
        m_resumeService = false;
        m_log->appendPlainText(tr("Запускаю zapret2 обратно…"));
        m_backend->start();
    }
    emit scanFinished(m_found, canceled);
}
