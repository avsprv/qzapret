#include "updatepage.h"

#include <QDateTime>
#include <QDesktopServices>
#include <QFontDatabase>
#include <QGridLayout>
#include <QGroupBox>
#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPlainTextEdit>
#include <QPushButton>
#include <QSettings>
#include <QTimer>
#include <QUrl>
#include <QVBoxLayout>

#include "appsettings.h"
#include "backend/helperpath.h"
#include "backend/updatechecker.h"
#include "backend/zapretbackend.h"
#include "backend/zapretpaths.h"
#include "backend/zapretupdater.h"

namespace {
constexpr qint64 kAutoCheckIntervalSecs = 24 * 60 * 60;
constexpr int kTimerIntervalMs = 6 * 60 * 60 * 1000;
constexpr int kStartupDelayMs = 8000;
}

UpdatePage::UpdatePage(ZapretBackend *backend, QWidget *parent)
    : QWidget(parent)
    , m_backend(backend)
    , m_checker(new UpdateChecker(this))
    , m_updater(new ZapretUpdater(this))
    , m_timer(new QTimer(this))
{
    m_installedLabel = new QLabel;
    m_latestLabel = new QLabel(QStringLiteral("—"));
    m_statusLabel = new QLabel;
    m_statusLabel->setWordWrap(true);
    m_checkBtn = new QPushButton(tr("Проверить обновления"));
    m_updateBtn = new QPushButton;
    m_notesBtn = new QPushButton(tr("Что нового…"));

    m_log = new QPlainTextEdit;
    m_log->setReadOnly(true);
    m_log->setMaximumBlockCount(2000);
    m_log->setFont(QFontDatabase::systemFont(QFontDatabase::FixedFont));

    auto *note = new QLabel(tr("Файлы скачиваются с github.com/bol-van/zapret2. Проверяются sha256-суммы "
                               "бинарников из sha256sum.txt релиза; подписи у апстрима нет, поэтому это защита "
                               "от повреждения файлов, а не от компрометации репозитория. Конфигурация и ваши "
                               "списки сохраняются, копия прежней версии остаётся в «zapret2.bak»."));
    note->setWordWrap(true);

    auto *group = new QGroupBox(QStringLiteral("zapret2"));
    auto *grid = new QGridLayout(group);
    grid->addWidget(new QLabel(tr("Установлена:")), 0, 0);
    grid->addWidget(m_installedLabel, 0, 1);
    grid->addWidget(new QLabel(tr("Последняя версия:")), 1, 0);
    grid->addWidget(m_latestLabel, 1, 1);
    grid->setColumnStretch(1, 1);

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_checkBtn);
    buttons->addWidget(m_updateBtn);
    buttons->addWidget(m_notesBtn);
    buttons->addStretch();

    auto *layout = new QVBoxLayout(this);
    layout->addWidget(group);
    layout->addWidget(m_statusLabel);
    layout->addLayout(buttons);
    layout->addWidget(note);
    layout->addWidget(new QLabel(tr("Журнал обновления:")));
    layout->addWidget(m_log, 1);

    connect(m_checkBtn, &QPushButton::clicked, this, [this] { checkNow(false); });
    connect(m_updateBtn, &QPushButton::clicked, this, [this] {
        if (m_installed.isEmpty())
            confirmAndInstall();
        else
            confirmAndUpdate();
    });
    connect(m_notesBtn, &QPushButton::clicked, this, [this] {
        if (!m_latestTag.isEmpty())
            QDesktopServices::openUrl(QUrl(ZapretVersion::releaseUrl(m_latestTag)));
    });

    connect(m_checker, &UpdateChecker::finished, this, [this](const QString &tag) {
        m_latestTag = tag;
        QSettings s;
        s.setValue(QStringLiteral("update/lastCheck"), QDateTime::currentSecsSinceEpoch());
        const bool newer = !m_installed.isEmpty() && ZapretVersion::compare(m_installed, tag) < 0;
        if (m_automaticCheck && newer && s.value(QStringLiteral("update/notifiedTag")).toString() != tag) {
            s.setValue(QStringLiteral("update/notifiedTag"), tag);
            emit updateAvailable(tag);
        }
        refreshUi();
    });
    connect(m_checker, &UpdateChecker::failed, this, [this](const QString &msg) {
        // автоматическая проверка молчит, ручная — показывает причину
        if (m_automaticCheck) {
            refreshUi();
            return;
        }
        refreshUi();
        m_statusLabel->setText(tr("Не удалось проверить обновления: %1").arg(msg));
    });

    connect(m_updater, &ZapretUpdater::logLine, m_log, &QPlainTextEdit::appendPlainText);
    connect(m_updater, &ZapretUpdater::failedToStart, this, [this](const QString &msg) {
        emit busyChanged(false);
        refreshUi();
        QMessageBox::warning(this, tr("Ошибка"), msg);
    });
    connect(m_updater, &ZapretUpdater::finished, this, &UpdatePage::onUpdateFinished);

    refreshInstalled();
    refreshUi();

    // автопроверка: через несколько секунд после старта и затем раз в 6 часов (но не чаще раза в сутки)
    connect(m_timer, &QTimer::timeout, this, &UpdatePage::maybeAutoCheck);
    m_timer->start(kTimerIntervalMs);
    QTimer::singleShot(kStartupDelayMs, this, &UpdatePage::maybeAutoCheck);
}

bool UpdatePage::isBusy() const
{
    return m_updater->isRunning();
}

void UpdatePage::setServiceBusy(bool busy)
{
    m_serviceBusy = busy;
    refreshUi();
}

void UpdatePage::maybeAutoCheck()
{
    if (!AppSettings::autoCheckUpdates() || isBusy() || m_checker->isRunning())
        return;
    const qint64 last = QSettings().value(QStringLiteral("update/lastCheck"), 0).toLongLong();
    if (QDateTime::currentSecsSinceEpoch() - last < kAutoCheckIntervalSecs)
        return;
    checkNow(true);
}

void UpdatePage::checkNow(bool automatic)
{
    if (m_checker->isRunning() || isBusy())
        return;
    m_automaticCheck = automatic;
    refreshInstalled();
    m_statusLabel->setText(tr("Проверяю обновления…"));
    m_checkBtn->setEnabled(false);
    m_updateBtn->setEnabled(false);
    m_checker->check();
}

void UpdatePage::refreshInstalled()
{
    m_installed = ZapretVersion::installed(ZapretPaths::baseDir());
}

void UpdatePage::refreshUi()
{
    m_installedLabel->setText(m_installed.isEmpty() ? tr("не найдена в %1").arg(ZapretPaths::baseDir())
                                                    : QStringLiteral("v") + m_installed);
    m_latestLabel->setText(m_latestTag.isEmpty() ? QStringLiteral("—") : m_latestTag);

    const bool busy = isBusy();
    const bool known = !m_installed.isEmpty() && !m_latestTag.isEmpty();
    const bool newer = known && ZapretVersion::compare(m_installed, m_latestTag) < 0;

    if (m_installed.isEmpty())
        m_statusLabel->setText(m_latestTag.isEmpty()
                                   ? tr("zapret2 не установлен. Проверьте наличие релиза для установки.")
                                   : tr("zapret2 не установлен. Доступна чистая установка %1.").arg(m_latestTag));
    else if (m_latestTag.isEmpty())
        m_statusLabel->setText(tr("Версия на GitHub ещё не проверялась."));
    else if (newer)
        m_statusLabel->setText(tr("Доступна новая версия: %1.").arg(m_latestTag));
    else
        m_statusLabel->setText(tr("Установлена актуальная версия."));

    const bool canInstall = m_installed.isEmpty() && !m_latestTag.isEmpty();
    m_updateBtn->setText(canInstall ? tr("Установить %1").arg(m_latestTag)
                                    : (newer ? tr("Обновить до %1").arg(m_latestTag) : tr("Обновить")));
    m_updateBtn->setEnabled((canInstall || newer) && !busy && !m_checker->isRunning() && !m_serviceBusy);
    m_checkBtn->setEnabled(!busy && !m_checker->isRunning());
    m_notesBtn->setEnabled(!m_latestTag.isEmpty());
}

void UpdatePage::confirmAndUpdate()
{
    if (m_latestTag.isEmpty() || m_installed.isEmpty() || isBusy() || m_checker->isRunning())
        return;
    if (m_serviceBusy) {
        QMessageBox::information(this, tr("Идёт подбор"),
                                 tr("Дождитесь завершения подбора стратегий перед обновлением."));
        return;
    }
    const auto answer = QMessageBox::question(
        this, tr("Обновление zapret2"),
        tr("Обновить zapret2 с v%1 до %2?\n\n"
           "Если сервис запущен, он будет остановлен на время обновления и запущен снова. "
           "Потребуется пароль администратора.")
            .arg(m_installed, m_latestTag));
    if (answer != QMessageBox::Yes)
        return;

    m_log->clear();
    m_statusLabel->setText(tr("Обновляю до %1…").arg(m_latestTag));
    m_updateBtn->setEnabled(false);
    m_checkBtn->setEnabled(false);
    emit busyChanged(true);
    m_updater->start(m_latestTag);
}

void UpdatePage::confirmAndInstall()
{
    if (m_latestTag.isEmpty() || !m_installed.isEmpty() || isBusy() || m_checker->isRunning())
        return;
    if (m_serviceBusy) {
        QMessageBox::information(this, tr("Идёт подбор"),
                                 tr("Дождитесь завершения подбора стратегий перед установкой."));
        return;
    }
    const auto answer = QMessageBox::question(
        this, tr("Чистая установка zapret2"),
        tr("Установить zapret2 %1 в /opt/zapret2?\n\n"
           "Будет проверена структура архива и SHA-256 бинарных компонентов, создана конфигурация по умолчанию "
           "с NFQWS2_ENABLE=0 и установлен systemd unit.\n\n"
           "Служба не будет запущена или добавлена в автозагрузку. "
           "Сетевой трафик не будет перенаправляться. Потребуется пароль администратора.")
            .arg(m_latestTag));
    if (answer != QMessageBox::Yes)
        return;

    m_installing = true;
    m_log->clear();
    m_statusLabel->setText(tr("Устанавливаю zapret2 %1…").arg(m_latestTag));
    m_updateBtn->setEnabled(false);
    m_checkBtn->setEnabled(false);
    emit busyChanged(true);
    m_updater->startInstall(m_latestTag);
}

void UpdatePage::onUpdateFinished(int code)
{
    emit busyChanged(false);
    const bool wasInstalling = m_installing;
    m_installing = false;
    refreshInstalled();
    refreshUi();
    m_backend->refresh();

    if (code == 0)
        m_statusLabel->setText(wasInstalling ? tr("zapret2 установлен: %1.").arg(m_latestTag)
                                             : tr("zapret2 обновлён до %1.").arg(m_latestTag));
    else if (Helper::isAuthDismissed(code))
        m_statusLabel->setText(wasInstalling ? tr("Установка отменена: диалог авторизации закрыт.")
                                             : tr("Обновление отменено: диалог авторизации закрыт."));
    else
        m_statusLabel->setText(wasInstalling
                                   ? tr("Установка не выполнена (код %1). Подробности — в журнале ниже.").arg(code)
                                   : tr("Обновление не выполнено (код %1). Подробности — в журнале ниже.").arg(code));
}
