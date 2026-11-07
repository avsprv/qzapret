#pragma once
#include <QByteArray>
#include <QVector>
#include <QWidget>
#include "profileanalyzer.h"
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
    void showProfiles();
    void addProfile();
    void editProfile();
    void copyProfile();
    void deleteProfile();
    void applyProfiles();
    bool editProfileDialog(ProfileAnalyzer::Profile *profile, const QString &title);
    int selectedProfile() const;
    QLabel *m_status = nullptr;
    QPushButton *m_refreshButton = nullptr;
    QPushButton *m_addButton = nullptr;
    QPushButton *m_editButton = nullptr;
    QPushButton *m_copyButton = nullptr;
    QPushButton *m_deleteButton = nullptr;
    QPushButton *m_applyButton = nullptr;
    QTreeWidget *m_tree = nullptr;
    QProcess *m_process = nullptr;
    QByteArray m_configBytes;
    QString m_config;
    QVector<ProfileAnalyzer::Profile> m_profiles;
    bool m_loaded = false;
    bool m_dirty = false;
};
