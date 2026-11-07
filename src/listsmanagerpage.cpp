#include "listsmanagerpage.h"
#include <QAbstractSocket>
#include <QComboBox>
#include <QHostAddress>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QRegularExpression>
#include <QSignalBlocker>
#include <QVBoxLayout>
#include "backend/helperpath.h"

ListsManagerPage::ListsManagerPage(QWidget *parent) : QWidget(parent)
{
    m_status = new QLabel(tr("Выберите пользовательский список и нажмите «Прочитать»."), this);
    m_status->setWordWrap(true);
    m_listSelector = new QComboBox(this);
    m_listSelector->addItem(tr("Исходный включающий хостлист — zapret-hosts-user.txt"), "zapret-hosts-user.txt");
    m_listSelector->addItem(tr("Исходный исключающий хостлист — zapret-hosts-user-exclude.txt"), "zapret-hosts-user-exclude.txt");
    m_listSelector->addItem(tr("Исходный IP-ban хостлист — zapret-hosts-user-ipban.txt"), "zapret-hosts-user-ipban.txt");
    m_listSelector->addItem(tr("Результирующий IPv4 — zapret-ip-user.txt"), "zapret-ip-user.txt");
    m_listSelector->addItem(tr("Результирующий IPv6 — zapret-ip-user6.txt"), "zapret-ip-user6.txt");
    m_listSelector->addItem(tr("Результирующий IPv4 exclude — zapret-ip-exclude.txt"), "zapret-ip-exclude.txt");
    m_listSelector->addItem(tr("Результирующий IPv6 exclude — zapret-ip-exclude6.txt"), "zapret-ip-exclude6.txt");
    m_listSelector->addItem(tr("Результирующий IP-ban IPv4 — zapret-ip-user-ipban.txt"), "zapret-ip-user-ipban.txt");
    m_listSelector->addItem(tr("Результирующий IP-ban IPv6 — zapret-ip-user-ipban6.txt"), "zapret-ip-user-ipban6.txt");

    m_readButton = new QPushButton(tr("Прочитать"), this);
    m_saveButton = new QPushButton(tr("Сохранить хостлист"), this);
    m_editor = new QPlainTextEdit(this);
    m_editor->setReadOnly(true);
    m_editor->setPlaceholderText(tr("Содержимое выбранного списка появится здесь."));
    m_editor->setLineWrapMode(QPlainTextEdit::NoWrap);

    auto *controls = new QHBoxLayout;
    controls->addWidget(m_listSelector, 1);
    controls->addWidget(m_readButton);
    controls->addWidget(m_saveButton);
    auto *note = new QLabel(
        tr("Редактировать можно только три исходных пользовательских хостлиста. Каждая непустая строка должна содержать одно доменное имя, IP-адрес или CIDR. Пустые строки допустимы; домены должны быть ASCII или punycode. Производные IP-файлы доступны только для просмотра: zapret2 пересоздаёт их при разрешении доменов. Перед сохранением helper создаёт резервную копию и атомарно заменяет файл. Сервис автоматически не перезапускается."),
        this);
    note->setWordWrap(true);
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_status);
    layout->addLayout(controls);
    layout->addWidget(note);
    layout->addWidget(m_editor, 1);

    connect(m_readButton, &QPushButton::clicked, this, &ListsManagerPage::readSelectedList);
    connect(m_saveButton, &QPushButton::clicked, this, &ListsManagerPage::saveSelectedList);
    connect(m_listSelector, &QComboBox::currentIndexChanged, this, [this](int index) {
        Q_UNUSED(index);
        if (m_dirty) {
            const auto answer = QMessageBox::question(this, tr("Несохранённые изменения"),
                tr("Текущий хостлист изменён. Отбросить изменения и переключиться?"),
                QMessageBox::Yes | QMessageBox::No, QMessageBox::No);
            if (answer != QMessageBox::Yes) {
                const QSignalBlocker blocker(m_listSelector);
                m_listSelector->setCurrentIndex(m_loadedIndex);
                return;
            }
        }
        m_loaded = false;
        m_dirty = false;
        m_loadedIndex = -1;
        m_editor->clear();
        m_status->setText(tr("Выбран другой список. Нажмите «Прочитать»."));
        setBusy(m_process != nullptr);
    });
    connect(m_editor, &QPlainTextEdit::textChanged, this, [this] {
        if (m_loaded)
            m_dirty = true;
        setBusy(m_process != nullptr);
    });
    setBusy(false);
}

bool ListsManagerPage::isEditableSelected() const
{
    return m_listSelector->currentIndex() >= 0 && m_listSelector->currentIndex() < 3;
}

void ListsManagerPage::setBusy(bool busy)
{
    m_listSelector->setEnabled(!busy);
    m_readButton->setEnabled(!busy);
    m_saveButton->setEnabled(!busy && m_loaded && m_dirty && isEditableSelected());
    m_editor->setReadOnly(busy || !m_loaded || !isEditableSelected());
}

QString ListsManagerPage::validateHostlist(const QString &content) const
{
    if (content.toUtf8().size() > 1024 * 1024)
        return tr("Файл превышает ограничение 1 MiB.");
    const QStringList lines = content.split(QLatin1Char('\n'));
    if (lines.size() > 50000)
        return tr("Слишком много строк (максимум 50000).");
    static const QRegularExpression labelPattern(
        QStringLiteral("^[A-Za-z0-9](?:[A-Za-z0-9-]{0,61}[A-Za-z0-9])?$"));
    for (int i = 0; i < lines.size(); ++i) {
        const QString entry = lines.at(i);
        if (entry.isEmpty())
            continue;
        if (entry.size() > 4095)
            return tr("Строка %1 длиннее 4095 символов.").arg(i + 1);
        for (const QChar ch : entry) {
            if (ch.category() == QChar::Other_Control || ch.isSpace())
                return tr("Строка %1 содержит пробельный или управляющий символ.").arg(i + 1);
        }
        if (QString::fromLatin1(entry.toLatin1()) != entry)
            return tr("Строка %1: используйте ASCII-домен или его punycode-представление.").arg(i + 1);
        QHostAddress address;
        if (address.setAddress(entry))
            continue;
        const auto subnet = QHostAddress::parseSubnet(entry);
        if (!subnet.first.isNull() && subnet.second >= 0 &&
            subnet.second <= (subnet.first.protocol() == QAbstractSocket::IPv4Protocol ? 32 : 128))
            continue;
        QString domain = entry;
        if (domain.endsWith(QLatin1Char('.')))
            domain.chop(1);
        if (domain.isEmpty() || domain.size() > 253)
            return tr("Домен в строке %1 имеет недопустимую длину.").arg(i + 1);
        const QStringList labels = domain.split(QLatin1Char('.'));
        for (const QString &label : labels) {
            if (label.size() > 63 || !labelPattern.match(label).hasMatch())
                return tr("Некорректный домен или IP/CIDR в строке %1: %2").arg(i + 1).arg(entry);
        }
    }
    return {};
}

void ListsManagerPage::readSelectedList()
{
    if (m_process)
        return;
    const QString name = m_listSelector->currentData().toString();
    if (name.isEmpty())
        return;
    m_output.clear();
    m_loaded = false;
    m_dirty = false;
    m_loadedIndex = -1;
    m_editor->clear();
    m_status->setText(tr("Читаю список %1…").arg(name));
    setBusy(true);
    auto *proc = new QProcess(this);
    m_process = proc;
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc] {
        m_output.append(proc->readAllStandardOutput());
        if (m_output.size() > 1024 * 1024) {
            proc->kill();
            m_status->setText(tr("Список превышает ограничение 1 MiB."));
        }
    });
    connect(proc, &QProcess::finished, this,
            [this, proc, name](int exitCode, QProcess::ExitStatus exitStatus) {
        m_output.append(proc->readAllStandardOutput());
        const QString error = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        m_process = nullptr;
        proc->deleteLater();
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_editor->setPlainText(QString::fromUtf8(m_output));
            m_loaded = true;
            m_dirty = false;
            m_loadedIndex = m_listSelector->currentIndex();
            m_status->setText(tr("Список %1 прочитан.").arg(name));
        } else if (exitStatus == QProcess::NormalExit && exitCode == 3 &&
                   error.contains(QStringLiteral("list file does not exist"))) {
            m_editor->clear();
            m_loaded = true;
            m_dirty = false;
            m_loadedIndex = m_listSelector->currentIndex();
            m_status->setText(tr("Файл %1 ещё не создан. Можно добавить записи и сохранить его.").arg(name));
        } else {
            m_editor->clear();
            m_loaded = false;
            m_status->setText(tr("Не удалось прочитать %1: %2").arg(name,
                error.isEmpty() ? tr("код завершения %1").arg(exitCode) : error));
        }
        setBusy(false);
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

void ListsManagerPage::saveSelectedList()
{
    if (m_process || !m_loaded || !m_dirty || !isEditableSelected())
        return;
    const QString name = m_listSelector->currentData().toString();
    const QByteArray bytes = m_editor->toPlainText().toUtf8();
    const QString validationError = validateHostlist(m_editor->toPlainText());
    if (!validationError.isEmpty()) {
        QMessageBox::warning(this, tr("Некорректный хостлист"), validationError);
        return;
    }
    if (QMessageBox::question(this, tr("Сохранить хостлист"),
        tr("Создать резервную копию и атомарно сохранить %1? Производные IP-списки будут обновлены только штатными механизмами zapret2. Сервис автоматически не перезапускается.")
            .arg(name),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes)
        return;
    setBusy(true);
    m_status->setText(tr("Проверяю и сохраняю %1…").arg(name));
    auto *proc = new QProcess(this);
    m_process = proc;
    connect(proc, &QProcess::started, this, [proc, bytes] {
        proc->write(bytes);
        proc->closeWriteChannel();
    });
    connect(proc, &QProcess::finished, this,
            [this, proc, name](int exitCode, QProcess::ExitStatus exitStatus) {
        const QString output = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        const QString error = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        m_process = nullptr;
        proc->deleteLater();
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_dirty = false;
            m_status->setText(output.isEmpty() ? tr("Список %1 сохранён; резервная копия создана.").arg(name) : output);
        } else {
            m_status->setText(tr("Не удалось сохранить %1: %2").arg(name,
                error.isEmpty() ? tr("код завершения %1").arg(exitCode) : error));
        }
        setBusy(false);
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
    Helper::start(proc, {QStringLiteral("lists-write"), name});
}
