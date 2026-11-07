#pragma once
#include <QByteArray>
#include <QWidget>
class QCheckBox;
class QComboBox;
class QLabel;
class QPlainTextEdit;
class QProcess;
class QPushButton;
class QTimer;
class QHideEvent;
class QShowEvent;

class LogMonitorPage : public QWidget
{
    Q_OBJECT
public:
    explicit LogMonitorPage(QWidget *parent = nullptr);
protected:
    void showEvent(QShowEvent *event) override;
    void hideEvent(QHideEvent *event) override;
private:
    void readLogs();
    void setBusy(bool busy);
    QLabel *m_status = nullptr;
    QComboBox *m_lineCount = nullptr;
    QCheckBox *m_autoRefresh = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QProcess *m_process = nullptr;
    QTimer *m_timer = nullptr;
    QByteArray m_output;
};
