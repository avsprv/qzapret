#include "mainwindow.h"

#include <QHBoxLayout>
#include <QLabel>
#include <QMessageBox>
#include <QPushButton>
#include <QTimer>
#include <QVBoxLayout>

#include "backend/systemdbackend.h"

MainWindow::MainWindow(QWidget *parent)
    : QMainWindow(parent)
    , m_backend(new SystemdBackend(this))
    , m_status(new QLabel(tr("Статус: …"), this))
    , m_startBtn(new QPushButton(tr("Запустить"), this))
    , m_stopBtn(new QPushButton(tr("Остановить"), this))
    , m_poll(new QTimer(this))
{
    setWindowTitle(QStringLiteral("qzapret"));

    auto *buttons = new QHBoxLayout;
    buttons->addWidget(m_startBtn);
    buttons->addWidget(m_stopBtn);

    auto *layout = new QVBoxLayout;
    layout->addWidget(m_status);
    layout->addLayout(buttons);

    auto *central = new QWidget(this);
    central->setLayout(layout);
    setCentralWidget(central);

    connect(m_startBtn, &QPushButton::clicked, m_backend, &ZapretBackend::start);
    connect(m_stopBtn, &QPushButton::clicked, m_backend, &ZapretBackend::stop);
    connect(m_backend, &ZapretBackend::stateChanged, this, &MainWindow::onStateChanged);
    connect(m_backend, &ZapretBackend::errorOccurred, this, [this](const QString &msg) {
        QMessageBox::warning(this, tr("Ошибка"), msg);
    });

    m_poll->setInterval(3000);
    connect(m_poll, &QTimer::timeout, m_backend, &ZapretBackend::refresh);
    m_poll->start();
    m_backend->refresh();
}

void MainWindow::onStateChanged(ZapretBackend::State state)
{
    QString text;
    switch (state) {
    case ZapretBackend::State::Active:   text = tr("работает"); break;
    case ZapretBackend::State::Inactive: text = tr("остановлен"); break;
    case ZapretBackend::State::Failed:   text = tr("ошибка юнита"); break;
    default:                             text = tr("неизвестно (zapret2 не установлен?)"); break;
    }
    m_status->setText(tr("Статус: %1").arg(text));
    m_startBtn->setEnabled(state != ZapretBackend::State::Active);
    m_stopBtn->setEnabled(state == ZapretBackend::State::Active);
}
