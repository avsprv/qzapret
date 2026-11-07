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
public:
    explicit BlockcheckPage(ZapretBackend *backend, QWidget *parent = nullptr);
    bool isRunning() const { return m_runner->isRunning() || m_waitingStop; }

signals:
    void scanFinished(int found, bool canceled);

private:
    void onStartClicked();
    void beginScan();
    void onBackendState(ZapretBackend::State state);
    void onFinished(int exitCode, bool canceled);
    void saveLog();
    void setBusy(bool busy);
    BlockcheckRunner::Options collectOptions() const;

    ZapretBackend *m_backend;
    BlockcheckRunner *m_runner;
    ZapretBackend::State m_state = ZapretBackend::State::Unknown;
    bool m_waitingStop = false;
    bool m_resumeService = false;
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
