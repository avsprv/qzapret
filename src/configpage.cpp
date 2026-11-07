#include "configpage.h"

#include <QCryptographicHash>
#include <QDateTime>
#include <QFileDialog>
#include <QFileInfo>
#include <QFontDatabase>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QSaveFile>
#include <QTemporaryFile>
#include <QVBoxLayout>

#include "backend/helperpath.h"

ConfigPage::ConfigPage(QWidget *parent)
    : QWidget(parent)
{
    m_status = new QLabel(tr("Конфигурация ещё не загружена."));
    m_status->setWordWrap(true);
    m_readButton = new QPushButton(tr("Прочитать config"));
    m_validateButton = new QPushButton(tr("Проверить синтаксис"));
    m_saveButton = new QPushButton(tr("Сохранить в zapret2"));
    m_rollbackButton = new QPushButton(tr("Откатить последнюю копию"));
    m_exportButton = new QPushButton(tr("Экспортировать копию…"));
    m_editor = new QPlainTextEdit;
    m_editor->setReadOnly(false);
    m_editor->setEnabled(false);
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);
    m_editor->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));
    auto *note = new QLabel(tr("config — shell include. Проверка использует только sh -n: файл не исполняется и не загружается через source. Перед каждой записью создаётся резервная копия. Сервис автоматически не перезапускается."));
    note->setWordWrap(true);
    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_readButton);
    buttons->addWidget(m_validateButton);
    buttons->addWidget(m_saveButton);
    buttons->addWidget(m_rollbackButton);
    buttons->addWidget(m_exportButton);
    buttons->addStretch();
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_status);
    layout->addLayout(buttons);
    layout->addWidget(note);
    layout->addWidget(m_editor, 1);
    connect(m_readButton, &QPushButton::clicked, this, &ConfigPage::readConfig);
    connect(m_validateButton, &QPushButton::clicked, this, &ConfigPage::validateConfig);
    connect(m_saveButton, &QPushButton::clicked, this, &ConfigPage::saveConfig);
    connect(m_rollbackButton, &QPushButton::clicked, this, &ConfigPage::rollbackConfig);
    connect(m_exportButton, &QPushButton::clicked, this, &ConfigPage::exportConfig);
    setBusy(false);
    m_validateButton->setEnabled(false);
    m_saveButton->setEnabled(false);
    m_rollbackButton->setEnabled(false);
    m_exportButton->setEnabled(false);
}

void ConfigPage::setBusy(bool busy)
{
    m_readButton->setEnabled(!busy);
    m_validateButton->setEnabled(!busy && m_loaded);
    m_saveButton->setEnabled(!busy && m_loaded);
    m_rollbackButton->setEnabled(!busy && m_loaded);
    m_exportButton->setEnabled(!busy && m_loaded);
}

void ConfigPage::readConfig()
{
    if (m_process)
        return;
    m_loaded = false;
    m_configBytes.clear();
    m_editor->clear();
    m_editor->setEnabled(false);
    setBusy(true);
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
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_loaded = true;
            m_editor->setPlainText(QString::fromUtf8(m_configBytes));
            m_editor->setEnabled(true);
            setBusy(false);
            m_status->setText(tr("Конфигурация загружена. Можно редактировать и проверить перед записью."));
        } else {
            setBusy(false);
            const QString reason = error.isEmpty() ? tr("процесс завершился с кодом %1").arg(exitCode) : error;
            m_status->setText(tr("Не удалось прочитать config: %1").arg(reason));
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
    Helper::start(proc, {QStringLiteral("config-read")});
}

void ConfigPage::validateConfig()
{
    const QByteArray bytes = m_editor->toPlainText().toUtf8();
    if (bytes.size() > 1024 * 1024) {
        m_status->setText(tr("Конфигурация превышает ограничение 1 MiB."));
        return;
    }
    QTemporaryFile staged;
    if (!staged.open() || staged.write(bytes) != bytes.size() || !staged.flush()) {
        m_status->setText(tr("Не удалось подготовить временный файл для проверки: %1").arg(staged.errorString()));
        return;
    }
    QProcess validator;
    validator.start(QStringLiteral("/bin/sh"), {QStringLiteral("-n"), staged.fileName()});
    if (!validator.waitForStarted(3000) || !validator.waitForFinished(10000)) {
        validator.kill();
        validator.waitForFinished();
        m_status->setText(tr("Проверка shell-синтаксиса не завершилась."));
        return;
    }
    if (validator.exitStatus() == QProcess::NormalExit && validator.exitCode() == 0)
        m_status->setText(tr("Синтаксис корректен. Файл не исполнялся; системная конфигурация не изменена."));
    else
        m_status->setText(tr("Ошибка shell-синтаксиса: %1").arg(QString::fromUtf8(validator.readAllStandardError()).trimmed()));
}

void ConfigPage::saveConfig()
{
    if (m_process || !m_loaded)
        return;
    const QByteArray bytes = m_editor->toPlainText().toUtf8();
    if (bytes.size() > 1024 * 1024) {
        m_status->setText(tr("Конфигурация превышает ограничение 1 MiB."));
        return;
    }
    if (QMessageBox::question(this, tr("Сохранение конфигурации"),
        tr("Перед записью будет создана резервная копия. Проверить shell-синтаксис и атомарно заменить /opt/zapret2/config? Сервис автоматически не перезапустится."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    setBusy(true);
    m_status->setText(tr("Проверяю и сохраняю конфигурацию…"));
    auto *proc = new QProcess(this);
    m_process = proc;
    connect(proc, &QProcess::started, this, [proc, bytes] {
        proc->write(bytes);
        proc->closeWriteChannel();
    });
    connect(proc, &QProcess::finished, this,
            [this, proc, bytes](int exitCode, QProcess::ExitStatus exitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        const QString error = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        m_process = nullptr;
        proc->deleteLater();
        setBusy(false);
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_configBytes = bytes;
            m_status->setText(out.isEmpty() ? tr("Конфигурация сохранена; резервная копия создана.") : out);
        } else {
            const QString reason = error.isEmpty() ? tr("код завершения %1").arg(exitCode) : error;
            m_status->setText(tr("Не удалось сохранить конфигурацию: %1").arg(reason));
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
    const QString expectedHash = QString::fromLatin1(
        QCryptographicHash::hash(m_configBytes, QCryptographicHash::Sha256).toHex());
    Helper::start(proc, {QStringLiteral("config-write"), expectedHash});
}

void ConfigPage::rollbackConfig()
{
    if (m_process || !m_loaded)
        return;
    if (QMessageBox::question(this, tr("Откат конфигурации"),
        tr("Восстановить последнюю резервную копию /opt/zapret2/config? Текущая версия также будет сохранена в резервную копию."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    setBusy(true);
    m_status->setText(tr("Восстанавливаю резервную копию…"));
    auto *proc = new QProcess(this);
    m_process = proc;
    connect(proc, &QProcess::finished, this,
            [this, proc](int exitCode, QProcess::ExitStatus exitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        const QString error = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        m_process = nullptr;
        proc->deleteLater();
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_status->setText(out);
            readConfig();
        } else {
            setBusy(false);
            const QString reason = error.isEmpty() ? tr("код завершения %1").arg(exitCode) : error;
            m_status->setText(tr("Не удалось выполнить откат: %1").arg(reason));
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
    Helper::start(proc, {QStringLiteral("config-rollback")});
}

void ConfigPage::exportConfig()
{
    if (!m_loaded)
        return;
    const QString suggested = QStringLiteral("zapret2-config-%1.conf")
        .arg(QDateTime::currentDateTime().toString(QStringLiteral("yyyyMMdd-HHmmss")));
    const QString path = QFileDialog::getSaveFileName(this, tr("Экспортировать копию…"), suggested,
        tr("Конфигурация zapret2 (*.conf);;Все файлы (*)"));
    if (path.isEmpty())
        return;
    const QByteArray bytes = m_editor->toPlainText().toUtf8();
    QSaveFile file(path);
    if (!file.open(QIODevice::WriteOnly) || file.write(bytes) != bytes.size() || !file.commit()) {
        m_status->setText(tr("Не удалось сохранить копию: %1").arg(file.errorString()));
        return;
    }
    m_status->setText(tr("Копия конфигурации сохранена: %1").arg(QFileInfo(path).fileName()));
}
