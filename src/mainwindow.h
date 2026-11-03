#pragma once

#include <QMainWindow>

#include "backend/zapretbackend.h"

class QLabel;
class QPushButton;
class QTimer;

class MainWindow : public QMainWindow
{
    Q_OBJECT
public:
    explicit MainWindow(QWidget *parent = nullptr);

private:
    void onStateChanged(ZapretBackend::State state);

    ZapretBackend *m_backend;
    QLabel *m_status;
    QPushButton *m_startBtn;
    QPushButton *m_stopBtn;
    QTimer *m_poll;
};
