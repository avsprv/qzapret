#include "profilepage.h"
#include <QHeaderView>
#include <QHBoxLayout>
#include <QLabel>
#include <QProcess>
#include <QPushButton>
#include <QTreeWidget>
#include <QVBoxLayout>
#include "backend/helperpath.h"
#include "profileanalyzer.h"
ProfilePage::ProfilePage(QWidget *parent) : QWidget(parent)
{
    m_status = new QLabel(tr("Список профилей ещё не загружен."));
    m_status->setWordWrap(true);
    m_refreshButton = new QPushButton(tr("Обновить список"));
    m_tree = new QTreeWidget;
    m_tree->setColumnCount(2);
    m_tree->setHeaderLabels({tr("Параметр"), tr("Значение")});
    m_tree->header()->setSectionResizeMode(0, QHeaderView::ResizeToContents);
    m_tree->header()->setSectionResizeMode(1, QHeaderView::Stretch);
    m_tree->setAlternatingRowColors(true);
    auto *note = new QLabel(tr("Профили извлекаются из NFQWS2_OPT по разделителям --new. Анализатор принимает только статическую строку и никогда не исполняет config, shell-подстановки или Lua-код."));
    note->setWordWrap(true);
    auto *layout = new QVBoxLayout(this);
    auto *top = new QHBoxLayout;
    top->addWidget(m_refreshButton);
    top->addStretch();
    layout->addWidget(m_status);
    layout->addLayout(top);
    layout->addWidget(note);
    layout->addWidget(m_tree, 1);
    connect(m_refreshButton, &QPushButton::clicked, this, &ProfilePage::readConfig);
    setBusy(false);
}
void ProfilePage::setBusy(bool busy) { m_refreshButton->setEnabled(!busy); }
void ProfilePage::readConfig()
{
    if (m_process) return;
    m_tree->clear();
    m_configBytes.clear();
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
        setBusy(false);
        if (m_configBytes.size() > 1024 * 1024) {
            m_status->setText(tr("Анализ невозможен: %1").arg(tr("Слишком много профилей или параметров.")));
            return;
        }
        if (exitStatus != QProcess::NormalExit || exitCode != 0) {
            const QString reason = error.isEmpty() ? tr("процесс завершился с кодом %1").arg(exitCode) : error;
            m_status->setText(tr("Анализ невозможен: %1").arg(reason));
            return;
        }
        showAnalysis(QString::fromUtf8(m_configBytes));
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
void ProfilePage::showAnalysis(const QString &config)
{
    const auto result = ProfileAnalyzer::parse(config);
    if (!result.ok()) {
        m_status->setText(tr("Анализ невозможен: %1").arg(result.error));
        return;
    }
    for (qsizetype i = 0; i < result.profiles.size(); ++i) {
        const auto &profile = result.profiles.at(i);
        const QString summary = tr("Фильтры: %1; Payload: %2; Lua desync: %3")
            .arg(profile.filters.size()).arg(profile.payloads.size()).arg(profile.luaDesync.size());
        auto *root = new QTreeWidgetItem(m_tree,
            {tr("Профиль %1 — %2 параметров").arg(i + 1).arg(profile.allArguments.size()), summary});
        root->setExpanded(true);
        auto addGroup = [root](const QString &name, const QStringList &items) {
            if (items.isEmpty()) return;
            auto *group = new QTreeWidgetItem(root, {name, QString::number(items.size())});
            group->setExpanded(true);
            for (const QString &item : items) {
                const qsizetype eq = item.indexOf(QLatin1Char('='));
                const QString key = eq < 0 ? item : item.left(eq);
                const QString value = eq < 0 ? QString() : item.mid(eq + 1);
                new QTreeWidgetItem(group, {key, value});
            }
        };
        addGroup(tr("Фильтры"), profile.filters);
        addGroup(tr("Payload"), profile.payloads);
        addGroup(tr("Lua desync"), profile.luaDesync);
        addGroup(tr("Другие параметры"), profile.other);
    }
    m_status->setText(tr("Найдено профилей: %1. Статический анализ; конфигурация не исполнялась.")
                      .arg(result.profiles.size()));
}
