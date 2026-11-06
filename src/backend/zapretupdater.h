#pragma once

#include <QObject>

class QProcess;

// Обновление zapret2: `zapret2applet-helper update vX.Y.Z` через pkexec. Скачивание, проверка и
// установка — в хелпере (от root); здесь только запуск и разбор вывода.
class ZapretUpdater : public QObject
{
    Q_OBJECT
public:
    explicit ZapretUpdater(QObject *parent = nullptr);

    bool isRunning() const { return m_proc != nullptr; }
    void start(const QString &tag);

signals:
    void logLine(const QString &line);
    void finished(int exitCode);               // 0 — успех, 126 — диалог polkit закрыт, -1 — не запустился
    void failedToStart(const QString &message);

private:
    void drain();

    QProcess *m_proc = nullptr;
    QByteArray m_buf;
};
