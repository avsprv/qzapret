#include "logmonitorpage.h"

#include <QCheckBox>
#include <QComboBox>
#include <QDateTime>
#include <QHideEvent>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QScrollBar>
#include <QShowEvent>
#include <QTimer>
#include <QVBoxLayout>

#include "backend/helperpath.h"

LogMonitorPage::LogMonitorPage(QWidget *parent) : QWidget(parent)
{
    m_status = new QLabel(tr("Журнал службы ещё не загружен."), this);
    m_status->setWordWrap(true);

    m_lineCount = new QComboBox(this);
    m_lineCount->addItem(tr("Последние 50 строк"), QStringLiteral("50"));
    m_lineCount->addItem(tr("Последние 100 строк"), QStringLiteral("100"));
    m_lineCount->addItem(tr("Последние 200 строк"), QStringLiteral("200"));
    m_lineCount->addItem(tr("Последние 500 строк"), QStringLiteral("500"));
    m_lineCount->setCurrentIndex(2);

    m_refreshButton = new QPushButton(tr("Обновить журнал"), this);
    m_autoRefresh = new QCheckBox(tr("Автообновление каждые 10 секунд"), this);
    m_autoRefresh->setChecked(false);

    m_editor = new QPlainTextEdit(this);
    m_editor->setReadOnly(true);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_editor->setMaximumBlockCount(600);
    m_editor->setPlaceholderText(tr("Здесь появятся последние записи журнала zapret2.service."));

    auto *controls = new QHBoxLayout;
    controls->addWidget(m_lineCount);
    controls->addWidget(m_refreshButton);
    controls->addWidget(m_autoRefresh);
    controls->addStretch();

    auto *note = new QLabel(
        tr("Отображается только журнал фиксированной службы zapret2.service. Чтение выполняется через отдельное read-only действие Polkit; произвольные имена служб и параметры journalctl не передаются. Автообновление выключено по умолчанию. Данные не записываются и не изменяют состояние службы."),
        this);
    note->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_status);
    layout->addLayout(controls);
    layout->addWidget(note);
    layout->addWidget(m_editor, 1);

    m_timer = new QTimer(this);
    m_timer->setInterval(10000);
    connect(m_refreshButton, &QPushButton::clicked, this, &LogMonitorPage::readLogs);
    connect(m_timer, &QTimer::timeout, this, &LogMonitorPage::readLogs);
    connect(m_autoRefresh, &QCheckBox::toggled, this, [this](bool enabled) {
        if (enabled && isVisible()) {
            m_timer->start();
            readLogs();
        } else {
            m_timer->stop();
        }
    });
    setBusy(false);
}

void LogMonitorPage::setBusy(bool busy)
{
    m_lineCount->setEnabled(!busy);
    m_refreshButton->setEnabled(!busy);
}

void LogMonitorPage::readLogs()
{
    if (m_process)
        return;

    m_output.clear();
    m_status->setText(tr("Читаю журнал zapret2.service…"));
    setBusy(true);

    auto *proc = new QProcess(this);
    m_process = proc;
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc] {
        m_output.append(proc->readAllStandardOutput());
        if (m_output.size() > 1024 * 1024)
            proc->kill();
    });
    connect(proc, &QProcess::finished, this,
            [this, proc](int exitCode, QProcess::ExitStatus exitStatus) {
        m_output.append(proc->readAllStandardOutput());
        const QString error = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        m_process = nullptr;
        proc->deleteLater();
        setBusy(false);

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            auto *bar = m_editor->verticalScrollBar();
            const bool wasAtBottom = bar->value() >= bar->maximum() - 1;
            m_editor->setPlainText(QString::fromUtf8(m_output));
            if (wasAtBottom)
                bar->setValue(bar->maximum());
            m_status->setText(tr("Журнал обновлён: %1; получено %2 байт.")
                .arg(QDateTime::currentDateTime().toString(QStringLiteral("HH:mm:ss")))
                .arg(m_output.size()));
        } else {
            const QString reason = error.isEmpty()
                ? tr("код завершения %1").arg(exitCode) : error;
            m_status->setText(tr("Не удалось прочитать журнал: %1").arg(reason));
        }
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        const QString reason = proc->errorString();
        m_process = nullptr;
        proc->deleteLater();
        setBusy(false);
        m_status->setText(tr("Не удалось запустить хелпер: %1").arg(reason));
    });

    Helper::start(proc, {QStringLiteral("logs-read"), m_lineCount->currentData().toString()});
}

void LogMonitorPage::showEvent(QShowEvent *event)
{
    QWidget::showEvent(event);
    readLogs();
    if (m_autoRefresh->isChecked())
        m_timer->start();
}

void LogMonitorPage::hideEvent(QHideEvent *event)
{
    m_timer->stop();
    QWidget::hideEvent(event);
}