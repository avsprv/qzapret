#include "profilepage.h"

#include <QDialog>
#include <QDialogButtonBox>
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QProcess>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>

#include "backend/helperpath.h"

ProfilePage::ProfilePage(QWidget *parent) : QWidget(parent)
{
    m_status = new QLabel(tr("Список профилей ещё не загружен."));
    m_status->setWordWrap(true);
    m_refreshButton = new QPushButton(tr("Обновить список"));
    m_addButton = new QPushButton(tr("Создать"));
    m_editButton = new QPushButton(tr("Изменить"));
    m_copyButton = new QPushButton(tr("Копировать"));
    m_deleteButton = new QPushButton(tr("Удалить"));
    m_applyButton = new QPushButton(tr("Применить изменения"));
    m_tree = new QTreeWidget;
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({tr("Профиль / параметр"), tr("Значение")});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tree->setAlternatingRowColors(true);
    m_tree->setSelectionMode(QAbstractItemView::SingleSelection);
    auto *note = new QLabel(tr("Профили извлекаются из NFQWS2_OPT по разделителям --new. Редактор изменяет только статическое значение NFQWS2_OPT; config, shell-код и Lua не исполняются. Перед применением helper проверит shell-синтаксис, создаст резервную копию и атомарно заменит файл. Сервис автоматически не перезапускается."));
    note->setWordWrap(true);
    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_refreshButton);
    buttons->addWidget(m_addButton);
    buttons->addWidget(m_editButton);
    buttons->addWidget(m_copyButton);
    buttons->addWidget(m_deleteButton);
    buttons->addStretch();
    auto *layout = new QVBoxLayout(this);
    layout->addWidget(m_status);
    layout->addLayout(buttons);
    layout->addWidget(note);
    layout->addWidget(m_tree, 1);
    layout->addWidget(m_applyButton);
    connect(m_refreshButton, &QPushButton::clicked, this, &ProfilePage::readConfig);
    connect(m_addButton, &QPushButton::clicked, this, &ProfilePage::addProfile);
    connect(m_editButton, &QPushButton::clicked, this, &ProfilePage::editProfile);
    connect(m_copyButton, &QPushButton::clicked, this, &ProfilePage::copyProfile);
    connect(m_deleteButton, &QPushButton::clicked, this, &ProfilePage::deleteProfile);
    connect(m_applyButton, &QPushButton::clicked, this, &ProfilePage::applyProfiles);
    connect(m_tree, &QTreeWidget::itemSelectionChanged, this, [this] { setBusy(m_process != nullptr); });
    setBusy(false);
}
void ProfilePage::setBusy(bool busy)
{
    m_refreshButton->setEnabled(!busy);
    m_addButton->setEnabled(!busy && m_loaded);
    const bool selected = selectedProfile() >= 0;
    m_editButton->setEnabled(!busy && selected);
    m_copyButton->setEnabled(!busy && selected && m_profiles.size() < 128);
    m_deleteButton->setEnabled(!busy && selected && m_profiles.size() > 1);
    m_applyButton->setEnabled(!busy && m_loaded && m_dirty && !m_profiles.isEmpty());
}
int ProfilePage::selectedProfile() const
{
    const auto items = m_tree->selectedItems();
    if (items.isEmpty()) return -1;
    const int index = items.first()->data(0, Qt::UserRole).toInt();
    return index >= 0 && index < m_profiles.size() ? index : -1;
}
void ProfilePage::readConfig()
{
    if (m_process) return;
    m_tree->clear();
    m_configBytes.clear();
    m_loaded = false;
    m_dirty = false;
    setBusy(true);
    m_status->setText(tr("Читаю конфигурацию для анализа профилей…"));
    auto *proc = new QProcess(this);
    m_process = proc;
    connect(proc, &QProcess::readyReadStandardOutput, this, [this, proc] {
        m_configBytes.append(proc->readAllStandardOutput());
        if (m_configBytes.size() > 1024 * 1024) proc->kill();
    });
    connect(proc, &QProcess::finished, this,
            [this, proc](int exitCode, QProcess::ExitStatus exitStatus) {
        m_configBytes.append(proc->readAllStandardOutput());
        const QString error = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        m_process = nullptr;
        proc->deleteLater();
        if (m_configBytes.size() > 1024 * 1024) {
            m_status->setText(tr("Анализ невозможен: %1").arg(tr("Слишком много профилей или параметров.")));
            setBusy(false);
            return;
        }
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            const QString reason = error.isEmpty() ? tr("процесс завершился с кодом %1").arg(exitCode) : error;
            m_status->setText(tr("Анализ невозможен: %1").arg(reason));
            setBusy(false);
            return;
        }
        m_config = QString::fromUtf8(m_configBytes);
        const auto result = ProfileAnalyzer::parse(m_config);
        if (!result.ok()) {
            m_profiles.clear();
            m_status->setText(tr("Анализ невозможен: %1").arg(result.error));
            setBusy(false);
            return;
        }
        m_profiles = result.profiles;
        m_loaded = true;
        showProfiles();
        m_status->setText(tr("Найдено профилей: %1. Статический анализ; конфигурация не исполнялась.").arg(m_profiles.size()));
        setBusy(false);
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        const QString reason = proc->errorString();
        m_process = nullptr;
        proc->deleteLater();
        setBusy(false);
        m_status->setText(tr("Анализ невозможен: %1").arg(tr("Не удалось запустить хелпер: %1").arg(reason)));
    });
    Helper::start(proc, {QStringLiteral("config-read")});
}
void ProfilePage::showProfiles()
{
    m_tree->clear();
    for (int i = 0; i < m_profiles.size(); ++i) {
        const auto &profile = m_profiles.at(i);
        auto *root = new QTreeWidgetItem(m_tree,
            {tr("Профиль %1 — %2 параметров").arg(i + 1).arg(profile.allArguments.size()),
             tr("Фильтры: %1; Payload: %2; Lua desync: %3")
                 .arg(profile.filters.size()).arg(profile.payloads.size()).arg(profile.luaDesync.size())});
        root->setData(0, Qt::UserRole, i);
        root->setExpanded(true);
        for (const QString &argument : profile.allArguments)
            new QTreeWidgetItem(root, {argument, QString()});
    }
    setBusy(m_process != nullptr);
}
bool ProfilePage::editProfileDialog(ProfileAnalyzer::Profile *profile, const QString &title)
{
    if (!profile) return false;
    QDialog dialog(this);
    dialog.setWindowTitle(title);
    dialog.resize(620, 420);
    auto *layout = new QVBoxLayout(&dialog);
    auto *hint = new QLabel(tr("Один аргумент на строку. Не добавляйте кавычки, shell-подстановки или разделитель --new. Пробелы внутри аргумента не поддерживаются."));
    hint->setWordWrap(true);
    auto *editor = new QPlainTextEdit(&dialog);
    editor->setPlaceholderText(QStringLiteral("--filter-tcp=443\n--filter-l7=tls\n--payload=tls_client_hello"));
    editor->setPlainText(profile->allArguments.join(QLatin1Char('\n')));
    layout->addWidget(hint);
    layout->addWidget(editor, 1);
    auto *buttons = new QDialogButtonBox(QDialogButtonBox::Save | QDialogButtonBox::Cancel, &dialog);
    layout->addWidget(buttons);
    connect(buttons, &QDialogButtonBox::accepted, &dialog, &QDialog::accept);
    connect(buttons, &QDialogButtonBox::rejected, &dialog, &QDialog::reject);
    if (dialog.exec() != QDialog::Accepted) return false;
    QStringList args;
    const QStringList lines = editor->toPlainText().split(QLatin1Char('\n'));
    for (const QString &line : lines) {
        const QString arg = line.trimmed();
        if (!arg.isEmpty()) args.append(arg);
    }
    if (args.isEmpty()) {
        QMessageBox::warning(this, tr("Некорректный профиль"), tr("Профиль должен содержать хотя бы один аргумент."));
        return false;
    }
    if (args.contains(QStringLiteral("--new"))) {
        QMessageBox::warning(this, tr("Некорректный профиль"), tr("Разделитель --new добавляется автоматически и не может быть аргументом профиля."));
        return false;
    }
    ProfileAnalyzer::Profile candidate;
    candidate.allArguments = args;
    for (const QString &arg : args) {
        if (arg.startsWith(QStringLiteral("--filter-"))) candidate.filters.append(arg);
        else if (arg.startsWith(QStringLiteral("--payload="))) candidate.payloads.append(arg);
        else if (arg.startsWith(QStringLiteral("--lua-desync="))) candidate.luaDesync.append(arg);
        else candidate.other.append(arg);
    }
    const auto validation = ProfileAnalyzer::replaceProfiles(m_config, [&] {
        auto copy = m_profiles;
        const int index = selectedProfile();
        if (index >= 0 && index < copy.size()) copy[index] = candidate;
        else copy.append(candidate);
        return copy;
    }());
    if (!validation.ok()) {
        QMessageBox::warning(this, tr("Некорректный профиль"), validation.error);
        return false;
    }
    *profile = candidate;
    return true;
}
void ProfilePage::addProfile()
{
    if (!m_loaded || m_profiles.size() >= 128) return;
    ProfileAnalyzer::Profile profile;
    if (!editProfileDialog(&profile, tr("Создать профиль"))) return;
    m_profiles.append(profile);
    m_dirty = true;
    showProfiles();
    m_tree->setCurrentItem(m_tree->topLevelItem(m_profiles.size() - 1));
    m_status->setText(tr("Изменения не сохранены. Нажмите «Применить изменения» для безопасной записи."));
    setBusy(false);
}
void ProfilePage::editProfile()
{
    const int index = selectedProfile();
    if (index < 0) return;
    auto candidate = m_profiles.at(index);
    // Validate against a copy with the selected profile replaced.
    const auto original = m_profiles;
    if (!editProfileDialog(&candidate, tr("Изменить профиль"))) return;
    m_profiles[index] = candidate;
    m_dirty = true;
    showProfiles();
    m_tree->setCurrentItem(m_tree->topLevelItem(index));
    m_status->setText(tr("Изменения не сохранены. Нажмите «Применить изменения» для безопасной записи."));
    setBusy(false);
}
void ProfilePage::copyProfile()
{
    const int index = selectedProfile();
    if (index < 0 || m_profiles.size() >= 128) return;
    auto copy = m_profiles.at(index);
    if (!editProfileDialog(&copy, tr("Копировать профиль"))) return;
    m_profiles.insert(index + 1, copy);
    m_dirty = true;
    showProfiles();
    m_tree->setCurrentItem(m_tree->topLevelItem(index + 1));
    m_status->setText(tr("Изменения не сохранены. Нажмите «Применить изменения» для безопасной записи."));
    setBusy(false);
}
void ProfilePage::deleteProfile()
{
    const int index = selectedProfile();
    if (index < 0 || m_profiles.size() <= 1) return;
    if (QMessageBox::question(this, tr("Удалить профиль"),
        tr("Удалить профиль %1? Порядок оставшихся профилей изменится.").arg(index + 1),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    m_profiles.removeAt(index);
    m_dirty = true;
    showProfiles();
    if (!m_profiles.isEmpty()) m_tree->setCurrentItem(m_tree->topLevelItem(qMin(index, m_profiles.size() - 1)));
    m_status->setText(tr("Изменения не сохранены. Нажмите «Применить изменения» для безопасной записи."));
    setBusy(false);
}
void ProfilePage::applyProfiles()
{
    if (m_process || !m_loaded || !m_dirty) return;
    const auto edited = ProfileAnalyzer::replaceProfiles(m_config, m_profiles);
    if (!edited.ok()) {
        m_status->setText(tr("Не удалось подготовить изменения: %1").arg(edited.error));
        return;
    }
    const QByteArray bytes = edited.config.toUtf8();
    if (QMessageBox::question(this, tr("Применить профили"),
        tr("Записать изменения NFQWS2_OPT? Перед записью helper проверит shell-синтаксис и создаст резервную копию. Остальной текст config будет сохранён. Сервис автоматически не перезапустится."),
        QMessageBox::Yes | QMessageBox::No, QMessageBox::No) != QMessageBox::Yes) return;
    setBusy(true);
    m_status->setText(tr("Проверяю и применяю профили…"));
    auto *proc = new QProcess(this);
    m_process = proc;
    connect(proc, &QProcess::started, this, [proc, bytes] {
        proc->write(bytes);
        proc->closeWriteChannel();
    });
    connect(proc, &QProcess::finished, this,
            [this, proc, edited](int exitCode, QProcess::ExitStatus exitStatus) {
        const QString out = QString::fromUtf8(proc->readAllStandardOutput()).trimmed();
        const QString error = QString::fromUtf8(proc->readAllStandardError()).trimmed();
        m_process = nullptr;
        proc->deleteLater();
        if (exitStatus == QProcess::NormalExit && exitCode == 0) {
            m_config = edited.config;
            m_configBytes = m_config.toUtf8();
            m_dirty = false;
            m_status->setText(out.isEmpty() ? tr("Профили применены; резервная копия создана. Сервис не перезапускался.") : out);
        } else {
            const QString reason = error.isEmpty() ? tr("код завершения %1").arg(exitCode) : error;
            m_status->setText(tr("Не удалось применить профили: %1").arg(reason));
        }
        setBusy(false);
    });
    connect(proc, &QProcess::errorOccurred, this, [this, proc](QProcess::ProcessError error) {
        if (error != QProcess::FailedToStart) return;
        const QString reason = proc->errorString();
        m_process = nullptr;
        proc->deleteLater();
        setBusy(false);
        m_status->setText(tr("Не удалось запустить хелпер: %1").arg(reason));
    });
    Helper::start(proc, {QStringLiteral("config-write")});
}
