#pragma once
#include <QByteArray>
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
    void saveSelectedList();
    void setBusy(bool busy);
    bool isEditableSelected() const;
    QString validateHostlist(const QString &content) const;
    QLabel *m_status = nullptr;
    QComboBox *m_listSelector = nullptr;
    QPlainTextEdit *m_editor = nullptr;
    QPushButton *m_readButton = nullptr;
    QPushButton *m_saveButton = nullptr;
    QProcess *m_process = nullptr;
    QByteArray m_output;
    bool m_loaded = false;
    bool m_dirty = false;
    int m_loadedIndex = -1;
};
