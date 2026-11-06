#pragma once

#include <QWidget>

class QCheckBox;
class QLabel;
class QPlainTextEdit;
class QPushButton;
class QTimer;
class UpdateChecker;
class ZapretUpdater;
class ZapretBackend;

// Вкладка «Обновления»: версия установленного zapret2, проверка новой версии на GitHub и
// обновление через привилегированный хелпер.
class UpdatePage : public QWidget
{
    Q_OBJECT
public:
    explicit UpdatePage(ZapretBackend *backend, QWidget *parent = nullptr);

    bool isBusy() const;

signals:
    // Автоматическая проверка нашла версию новее установленной (один раз на версию).
    void updateAvailable(const QString &tag);

private:
    void checkNow(bool automatic);
    void maybeAutoCheck();
    void refreshInstalled();
    void refreshUi();
    void confirmAndUpdate();
    void onUpdateFinished(int code);

    ZapretBackend *m_backend;
    UpdateChecker *m_checker;
    ZapretUpdater *m_updater;
    QTimer *m_timer;

    QLabel *m_installedLabel;
    QLabel *m_latestLabel;
    QLabel *m_statusLabel;
    QPushButton *m_checkBtn;
    QPushButton *m_updateBtn;
    QPushButton *m_notesBtn;
    QCheckBox *m_autoCheck;
    QPlainTextEdit *m_log;

    QString m_installed; // "1.0.6" или пусто
    QString m_latestTag; // "v1.0.7" или пусто
    bool m_automaticCheck = false;
};
