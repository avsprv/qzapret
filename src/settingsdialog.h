#pragma once

#include <QDialog>

class QCheckBox;
class QComboBox;

class SettingsDialog : public QDialog
{
    Q_OBJECT
public:
    explicit SettingsDialog(QWidget *parent = nullptr);

signals:
    void languageRestartRequired();

private slots:
    void applySettings();

private:
    QComboBox *m_language = nullptr;
    QComboBox *m_theme = nullptr;
    QCheckBox *m_autoCheck = nullptr;
    QCheckBox *m_guiAutostart = nullptr;
    QCheckBox *m_closeToTray = nullptr;
};
