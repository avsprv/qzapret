#pragma once

#include <QWidget>

#include "backend/blockcheckrunner.h"
#include "backend/zapretbackend.h"

class QCheckBox;
class QComboBox;
class QLineEdit;
class QListWidget;
class QPlainTextEdit;
class QPushButton;
class QSpinBox;
class QTimer;

// Вкладка «Подбор стратегий»: обёртка над blockcheck2.sh.
class BlockcheckPage : public QWidget
{
    Q_OBJECT
    enum class LifecyclePhase { Idle, Checking, WaitingForStop, Scanning, WaitingForStart };
public:
    explicit BlockcheckPage(ZapretBackend *backend, QWidget *parent = nullptr);
    bool isRunning() const { return m_phase != LifecyclePhase::Idle || m_runner->isRunning(); }

signals:
    void scanFinished(int found, bool canceled);
    void busyChanged(bool busy);

public slots:
    void setUpdateBusy(bool busy);

private:
    void onStartClicked();
    void beginScan();
    void onVerifiedState(ZapretBackend::State state);
    void onUnitActionFinished(const QString &action, ZapretBackend::State state, bool success);
    void onFinished(int exitCode, bool canceled);
    void onLifecycleTimeout();
    void restoreService(const QString &reason);
    void finishLifecycle(const QString &message = {});
    void saveLog();
    void setBusy(bool busy);
    BlockcheckRunner::Options collectOptions() const;

    ZapretBackend *m_backend;
    BlockcheckRunner *m_runner;
    LifecyclePhase m_phase = LifecyclePhase::Idle;
    bool m_resumeService = false;
    bool m_cancelRequested = false;
    bool m_scanStarted = false;
    bool m_updateBusy = false;
    int m_found = 0;
    BlockcheckRunner::Options m_lastOptions;

    QLineEdit *m_domains;
    QComboBox *m_ipvs;
    QCheckBox *m_http;
    QCheckBox *m_tls12;
    QCheckBox *m_tls13;
    QCheckBox *m_http3;
    QSpinBox *m_repeats;
    QComboBox *m_scan;
    QPushButton *m_startBtn;
    QPushButton *m_cancelBtn;
    QPushButton *m_copyBtn;
    QPushButton *m_saveLogBtn;
    QPlainTextEdit *m_log;
    QListWidget *m_results;
    QTimer *m_stopTimeout;
};
