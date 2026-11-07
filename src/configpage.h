#pragma once
#include <QByteArray>
#include <QWidget>
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QProcess;
class ConfigPage : public QWidget
{
    Q_OBJECT
public:
    explicit ConfigPage(QWidget *parent = nullptr);
private:
    void readConfig();
    void validateConfig();
    void saveConfig();
    void rollbackConfig();
    void exportConfig();
    void setBusy(bool busy);
    QLabel *m_status = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QPushButton *m_readButton = nullptr;
    QPushButton *m_validateButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QPushButton *m_rollbackButton = nullptr;
    QPushButton *m_exportButton = nullptr;
    QProcess *m_process = nullptr;
    QByteArray m_configBytes;
    bool m_loaded = false;
};
