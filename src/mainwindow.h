#pragma once

#include <QMainWindow>

#include "backend/zapretbackend.h"

class BlockcheckPage;
class UpdatePage;
class QCheckBox;
class QLabel;
class QPushButton;
class QSystemTrayIcon;
class QTimer;
class QAction;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);
    bool trayAvailable() const { return m_tray != nullptr; }

protected:
    void closeEvent(QCloseEvent *event) override;

private:
    QWidget *buildStatusPage();
    void buildTray();
    void onStateChanged(ZapretBackend::State state);
    void toggleWindow();
    void quit();

    ZapretBackend *m_backend;
    BlockcheckPage *m_blockcheck = nullptr;
    UpdatePage *m_update = nullptr;
    QLabel *m_backendInfo = nullptr;
    QLabel *m_status = nullptr;
    QPushButton *m_startBtn = nullptr;
    QPushButton *m_stopBtn = nullptr;
    QCheckBox *m_bootCheck = nullptr;
    QCheckBox *m_guiCheck = nullptr;
    QSystemTrayIcon *m_tray = nullptr;
    QAction *m_trayStart = nullptr;
    QAction *m_trayStop = nullptr;
    QTimer *m_poll = nullptr;
    bool m_quitting = false;
};
