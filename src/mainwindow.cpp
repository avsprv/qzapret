#include "mainwindow.h"

#include <QAction>
#include <QApplication>
#include <QCheckBox>
#include <QCloseEvent>
#include <QLabel>
#include <QMenu>
#include <QMessageBox>
#include <QPushButton>
#include <QStyle>
#include <QSystemTrayIcon>
#include <QTabWidget>
#include <QTimer>
#include <QVBoxLayout>

#include "autostart.h"
#include "backend/systemdbackend.h"
#include "blockcheckpage.h"
#include "configpage.h"
#include "listsmanagerpage.h"
#include "profilepage.h"
#include "updatepage.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_backend(new SystemdBackend(this))
    , m_poll(new QTimer(this))
{
    setWindowTitle(QStringLiteral("zapret2applet"));
    resize(760, 640);

    auto *tabs = new QTabWidget(this);
    tabs->addTab(buildStatusPage(), tr("Статус"));
    tabs->addTab(new ConfigPage(this), tr("Конфигурация"));
    tabs->addTab(new ProfilePage(this), tr("Профили"));
    tabs->addTab(new ListsManagerPage(this), tr("Списки"));
    m_blockcheck = new BlockcheckPage(m_backend, this);
    tabs->addTab(m_blockcheck, tr("Подбор стратегий"));
    m_update = new UpdatePage(m_backend, this);
    tabs->addTab(m_update, tr("Обновления"));
    connect(m_blockcheck, &BlockcheckPage::busyChanged, this, [this](bool busy) {
        m_update->setServiceBusy(busy);
        m_backend->refresh();
    });
    connect(m_update, &UpdatePage::busyChanged, this, [this](bool busy) {
        m_blockcheck->setUpdateBusy(busy);
        m_backend->refresh();
    });
    setCentralWidget(tabs);

    connect(m_backend, &ZapretBackend::stateChanged, this, &MainWindow::onStateChanged);
    connect(m_backend, &ZapretBackend::errorOccurred, this, [this](const QString &msg) {
        QMessageBox::warning(this, tr("Ошибка"), msg);
    });
    connect(m_backend, &ZapretBackend::bootEnabledChanged, this, [this](bool on) {
        const QSignalBlocker block(m_bootCheck);
        m_bootCheck->setChecked(on);
    });

    buildTray();
    connect(m_blockcheck, &BlockcheckPage::scanFinished, this, [this](int found, bool canceled) {
        if (m_tray && !isVisible() && !canceled)
            m_tray->showMessage(QStringLiteral("zapret2applet"),
                                tr("Подбор завершён, найдено стратегий: %1").arg(found));
    });

    connect(m_update, &UpdatePage::updateAvailable, this, [this](const QString &tag) {
        if (m_tray && !isVisible())
            m_tray->showMessage(QStringLiteral("zapret2applet"), tr("Доступна новая версия zapret2: %1").arg(tag));
    });

    m_poll->setInterval(3000);
    connect(m_poll, &QTimer::timeout, m_backend, &ZapretBackend::refresh);
    m_poll->start();
    m_backend->refresh();
}

QWidget *MainWindow::buildStatusPage()
{
    m_status = new QLabel(tr("Статус: …"));
    m_startBtn = new QPushButton(tr("Запустить"));
    m_stopBtn = new QPushButton(tr("Остановить"));
    m_bootCheck = new QCheckBox(tr("Запускать zapret2 при загрузке системы"));
    m_guiCheck = new QCheckBox(tr("Запускать zapret2applet при входе в сеанс (в трей)"));
    m_guiCheck->setChecked(Autostart::isEnabled());

    connect(m_startBtn, &QPushButton::clicked, this, [this] {
        if (m_blockcheck->isRunning() || m_update->isBusy()) {
            QMessageBox::information(this, tr("Идёт подбор"),
                                     tr("Дождитесь завершения подбора стратегий перед управлением сервисом."));
            return;
        }
        m_backend->start();
    });
    connect(m_stopBtn, &QPushButton::clicked, this, [this] {
        if (m_blockcheck->isRunning() || m_update->isBusy()) {
            QMessageBox::information(this, tr("Идёт подбор"),
                                     tr("Дождитесь завершения подбора стратегий перед управлением сервисом."));
            return;
        }
        m_backend->stop();
    });
    // clicked, а не toggled: состояние чекбокса потом приводится к реальному через refresh()
    connect(m_bootCheck, &QCheckBox::clicked, m_backend, &ZapretBackend::setBootEnabled);
    connect(m_guiCheck, &QCheckBox::clicked, this, [this](bool on) {
        if (!Autostart::setEnabled(on)) {
            QMessageBox::warning(this, tr("Ошибка"), tr("Не удалось изменить автозапуск zapret2applet."));
            const QSignalBlocker block(m_guiCheck);
            m_guiCheck->setChecked(Autostart::isEnabled());
        }
    });

    auto *page = new QWidget;
    auto *layout = new QVBoxLayout(page);
    layout->addWidget(m_status);
    auto *row = new QHBoxLayout;
    row->addWidget(m_startBtn);
    row->addWidget(m_stopBtn);
    row->addStretch();
    layout->addLayout(row);
    layout->addSpacing(12);
    layout->addWidget(m_bootCheck);
    layout->addWidget(m_guiCheck);
    layout->addStretch();
    return page;
}

void MainWindow::buildTray()
{
    if (!QSystemTrayIcon::isSystemTrayAvailable())
        return;

    m_tray = new QSystemTrayIcon(this);
    m_tray->setIcon(QIcon::fromTheme(QStringLiteral("security-low"),
                                     style()->standardIcon(QStyle::SP_ComputerIcon)));
    m_tray->setToolTip(QStringLiteral("zapret2applet"));

    auto *menu = new QMenu(this);
    auto *show = menu->addAction(tr("Показать / скрыть окно"));
    menu->addSeparator();
    m_trayStart = menu->addAction(tr("Запустить zapret2"));
    m_trayStop = menu->addAction(tr("Остановить zapret2"));
    menu->addSeparator();
    auto *quitAct = menu->addAction(tr("Выход"));

    connect(show, &QAction::triggered, this, &MainWindow::toggleWindow);
    connect(m_trayStart, &QAction::triggered, this, [this] {
        if (!m_blockcheck->isRunning() && !m_update->isBusy())
            m_backend->start();
    });
    connect(m_trayStop, &QAction::triggered, this, [this] {
        if (!m_blockcheck->isRunning() && !m_update->isBusy())
            m_backend->stop();
    });
    connect(quitAct, &QAction::triggered, this, &MainWindow::quit);
    connect(m_tray, &QSystemTrayIcon::activated, this, [this](QSystemTrayIcon::ActivationReason r) {
        if (r == QSystemTrayIcon::Trigger)
            toggleWindow();
    });

    m_tray->setContextMenu(menu);
    m_tray->show();
}

void MainWindow::onStateChanged(ZapretBackend::State state)
{
    QString text;
    QString iconName = QStringLiteral("security-low");
    switch (state) {
    case ZapretBackend::State::Active:
        text = tr("работает");
        iconName = QStringLiteral("security-high");
        break;
    case ZapretBackend::State::Inactive: text = tr("остановлен"); break;
    case ZapretBackend::State::Failed:   text = tr("ошибка юнита"); break;
    default:                             text = tr("неизвестно (zapret2 не установлен?)"); break;
    }
    m_status->setText(tr("Статус: %1").arg(text));
    const bool active = state == ZapretBackend::State::Active;
    const bool locked = m_blockcheck->isRunning() || m_update->isBusy();
    m_startBtn->setEnabled(!active && !locked);
    m_stopBtn->setEnabled(active && !locked);

    if (m_tray) {
        m_tray->setIcon(QIcon::fromTheme(iconName, style()->standardIcon(QStyle::SP_ComputerIcon)));
        m_tray->setToolTip(tr("zapret2applet: %1").arg(text));
        m_trayStart->setEnabled(!active && !locked);
        m_trayStop->setEnabled(active && !locked);
    }
}

void MainWindow::toggleWindow()
{
    if (isVisible() && !isMinimized()) {
        hide();
    } else {
        showNormal();
        raise();
        activateWindow();
    }
}

void MainWindow::quit()
{
    if (m_update->isBusy()) {
        showNormal();
        QMessageBox::information(this, tr("Идёт обновление"),
                                 tr("Дождитесь завершения обновления zapret2 перед выходом."));
        return;
    }
    if (m_blockcheck->isRunning()) {
        showNormal();
        QMessageBox::information(this, tr("Идёт подбор"),
                                 tr("Остановите подбор стратегий перед выходом."));
        return;
    }
    m_quitting = true;
    QApplication::quit();
}

void MainWindow::closeEvent(QCloseEvent *event)
{
    // Есть трей — крестик сворачивает в трей, выход через меню трея.
    if (m_tray && m_tray->isVisible() && !m_quitting) {
        hide();
        event->ignore();
        return;
    }
    if (m_update->isBusy()) {
        QMessageBox::information(this, tr("Идёт обновление"),
                                 tr("Дождитесь завершения обновления zapret2 перед выходом."));
        event->ignore();
        return;
    }
    if (m_blockcheck->isRunning()) {
        QMessageBox::information(this, tr("Идёт подбор"),
                                 tr("Остановите подбор стратегий перед выходом."));
        event->ignore();
        return;
    }
    event->accept();
}
