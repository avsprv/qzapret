#pragma once

#include <QWidget>

class QComboBox;
class QLabel;
class QPlainTextEdit;
class QProcess;
class QPushButton;

class ListsManagerPage : public QWidget
{
    Q_OBJECT
public:
    explicit ListsManagerPage(QWidget *parent = nullptr);

private:
    void readSelectedList();
    void setBusy(bool busy);

    QLabel *m_status = nullptr;
    QComboBox *m_listSelector = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QPushButton *m_readButton = nullptr;
    QProcess *m_process = nullptr;
};
