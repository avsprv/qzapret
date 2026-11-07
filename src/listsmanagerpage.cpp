#include "listsmanagerpage.h"

#include <QComboBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QTextCursor>
#include <QVBoxLayout>

#include "backend/helperpath.h"

ListsManagerPage::ListsManagerPage(QWidget *parent) : QWidget(parent)
{
    m_status = new QLabel(tr("Выберите пользовательский список и нажмите «Прочитать»."), this);
    m_status->setWordWrap(true);

    m_listSelector = new QComboBox(this);
    m_listSelector->addItem(tr("Включающий хостлист — zapret-hosts-user.txt"),
                            QStringLiteral("zapret-hosts-user.txt"));
    m_listSelector->addItem(tr("Исключающий хостлист — zapret-hosts-user-exclude.txt"),
                            QStringLiteral("zapret-hosts-user-exclude.txt"));
    m_listSelector->addItem(tr("Хостлист IP-ban — zapret-hosts-user-ipban.txt"),
                            QStringLiteral("zapret-hosts-user-ipban.txt"));
    m_listSelector->addItem(tr("Результирующий IPv4 — zapret-ip-user.txt"),
                            QStringLiteral("zapret-ip-user.txt"));
    m_listSelector->addItem(tr("Результирующий IPv6 — zapret-ip-user6.txt"),
                            QStringLiteral("zapret-ip-user6.txt"));
    m_listSelector->addItem(tr("Результирующий IPv4 exclude — zapret-ip-exclude.txt"),
                            QStringLiteral("zapret-ip-exclude.txt"));
    m_listSelector->addItem(tr("Результирующий IPv6 exclude — zapret-ip-exclude6.txt"),
                            QStringLiteral("zapret-ip-exclude6.txt"));
    m_listSelector->addItem(tr("Результирующий IP-ban IPv4 — zapret-ip-user-ipban.txt"),
                            QStringLiteral("zapret-ip-user-ipban.txt"));
    m_listSelector->addItem(tr("Результирующий IP-ban IPv6 — zapret-ip-user-ipban6.txt"),
                            QStringLiteral("zapret-ip-user-ipban6.txt"));

    m_readButton = new QPushButton(tr("Прочитать"), this);
    m_editor = new QPlainTextEdit(this);
    m_editor->setReadOnly(true);
    m_editor->setPlaceholderText(tr("Содержимое выбранного списка появится здесь."));
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);

    auto *controls = new QHBoxLayout;
    controls->addWidget(m_listSelector, 1);
    controls->addWidget(m_readButton);

    auto *note = new QLabel(
        tr("Отображаются только стандартные пользовательские файлы из /opt/zapret2/ipset. "
           "Генерируемые и загружаемые списки доступны только для чтения через другие механизмы "
           "zapret2 и не редактируются здесь. На этом этапе просмотр не изменяет файлы."),
        this);
    note->setWordWrap(true);

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_status);
    layout->addLayout(controls);
    layout->addWidget(note);
    layout->addWidget(m_editor, 1);

    connect(m_readButton, &QPushButton::clicked, this, &ListsManagerPage::readSelectedList);
    connect(m_listSelector, &QComboBox::currentIndexChanged, this, [this] {
        m_editor->clear();
        m_status->setText(tr("Выбран другой список. Нажмите «Прочитать»."));
    });
    setBusy(false);
}

void ListsManagerPage::setBusy(bool busy)
{
    m_listSelector->setEnabled(!busy);
    m_readButton->setEnabled(!busy);
}

void ListsManagerPage::readSelectedList()
{
    if (m_process)
        return;

    const QString name = m_listSelector->currentData().toString();
    if (name.isEmpty())
        return;

    m_editor->clear();
    m_status->setText(tr("Читаю список %1…").arg(name));
    setBusy(true);

    auto *proc = new QProcess(this);
    m_process = proc;
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc] {
        const QByteArray chunk = proc->readAllStandardOutput();
        if (m_editor->toPlainText().toUtf8().size() + chunk.size() > 1024 * 1024) {
            proc->kill();
            m_status->setText(tr("Список превышает ограничение 1 MiB."));
            return;
        }
        m_editor->moveCursor(QTextCursor::End);
        m_editor->insertPlainText(QString::fromUtf8(chunk));
    });
    connect(proc, &QProcess::finished, this,
            [this, proc, name](int exitCode, QProcess::ExitStatus exitStatus) {
        const QByteArray remaining = proc->readAllStandardOutput();
        if (!remaining.isEmpty()) {
            m_editor->moveCursor(QTextCursor::End);
            m_editor->insertPlainText(QString::fromUtf8(remaining));
        }
        const QString error = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        m_process = nullptr;
        proc->deleteLater();
        setBusy(false);

        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_status->setText(tr("Список %1 прочитан. Файл не изменялся.").arg(name));
        } else {
            m_editor->clear();
            m_status->setText(tr("Не удалось прочитать %1: %2").arg(name,
                error.isEmpty() ? tr("код завершения %1").arg(exitCode) : error));
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

    Helper::start(proc, {QStringLiteral("lists-read"), name});
}
