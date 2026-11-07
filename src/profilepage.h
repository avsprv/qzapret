#pragma once
#include <QWidget>
class QLabel;
class QPushButton;
class QTreeWidget;
class QProcess;
class ProfilePage : public QWidget {
    Q_OBJECT
public:
    explicit ProfilePage(QWidget *parent = nullptr);
private:
    void readConfig();
    void setBusy(bool busy);
    void showAnalysis(const QString &config);
    QLabel *m_status = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QTreeWidget *m_tree = nullptr;
    QProcess *m_process = nullptr;
    QByteArray m_configBytes;
};
