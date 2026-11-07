#include "configpage.h"

#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QVBoxLayout>

#include "backend/helperpath.h"

ConfigPage::ConfigPage(QWidget *parent)
    : QWidget(parent)
{
    m_status = new QLabel(tr("Конфигурация ещё не загружена."));
    m_status->setWordWrap(true);

    m_readButton = new QPushButton(tr("Прочитать config"));
    m_exportButton = new QPushButton(tr("Экспортировать копию…"));
    m_exportButton->setEnabled(false);

    m_editor = new QPlainTextEdit;
    m_editor->setReadOnly(true);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    auto *note = new QLabel(tr("Файл читается через привилегированный read-only helper. "
                               "Содержимое не исполняется и не интерпретируется приложением."));
    note->setWordWrap(true);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_readButton);
    buttons->addWidget(m_exportButton);
    buttons->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_status);
    layout->addLayout(buttons);
    layout->addWidget(note);
    layout->addWidget(m_editor, 1);

    connect(m_readButton, &QPushButton::clicked, this, &ConfigPage::readConfig);
    connect(m_exportButton, &QPushButton::clicked, this, &ConfigPage::exportConfig);
}

void ConfigPage::readConfig()
{
    if (m_process)
        return;

    m_loaded = false;
    m_configBytes.clear();
    m_editor->clear();
    m_exportButton->setEnabled(false);
    m_readButton->setEnabled(false);
    m_status->setText(tr("Читаю /opt/zapret2/config…"));

    auto *proc = new QProcess(this);
    m_process = proc;

    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc] {
        m_configBytes.append(proc->readAllStandardOutput());
    });
    connect(proc, &QProcess::finished, this,
            [this, proc](int exitCode, QProcess::ExitStatus exitStatus) {
        m_configBytes.append(proc->readAllStandardOutput());
        const QString error = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        m_process = nullptr;
        proc->deleteLater();
        m_readButton->setEnabled(true);

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_loaded = true;
            m_editor->setPlainText(QString::fromUtf8(m_configBytes));
            m_exportButton->setEnabled(true);
            m_status->setText(tr("Конфигурация загружена. Режим просмотра: файл не изменяется."));
        } else {
            const QString reason = error.isEmpty()
                ? tr("процесс завершился с кодом %1").arg(exitCode)
                : error;
            m_status->setText(tr("Не удалось прочитать config: %1").arg(reason));
        }
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart)
            return;
        const QString reason = proc->errorString();
        m_process = nullptr;
        proc->deleteLater();
        m_readButton->setEnabled(true);
        m_status->setText(tr("Не удалось запустить хелпер: %1").arg(reason));
    });

    Helper::start(proc, {QStringLiteral("config-read")});
}

void ConfigPage::exportConfig()
{
    if (!m_loaded) {
        m_status->setText(tr("Экспорт доступен после чтения конфигурации."));
        return;
    }

    const QString suggested = QStringLiteral("zapret2-config-%1.conf")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    const QString path = QFileDialog::getSaveFileName(
        this, tr("Экспортировать копию…"), suggested,
        tr("Конфигурация zapret2 (*.conf);;Все файлы (*)"));
    if (path.isEmpty())
        return;

    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(m_configBytes) != m_configBytes.size()
        || !file.commit()) {
        const QString reason = file.errorString();
        m_status->setText(tr("Не удалось сохранить копию: %1").arg(reason));
        return;
    }

    m_status->setText(tr("Копия конфигурации сохранена: %1").arg(QFileInfo(path).fileName()));
}
