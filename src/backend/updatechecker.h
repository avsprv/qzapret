#pragma once

#include <QObject>
#include <QString>

class QNetworkAccessManager;

// Версии zapret2: установленная (docs/changes.txt) и последняя (редирект
// https://github.com/bol-van/zapret2/releases/latest). GitHub API не используется:
// у него жёсткий лимит запросов без токена, а редирект лимитов не имеет.
namespace ZapretVersion {

// "v1.0.7" -> "1.0.7"
QString normalize(const QString &versionOrTag);

// <0, 0, >0. "1.0.10" > "1.0.9"; недостающие компоненты считаются нулём ("1.0" == "1.0.0").
int compare(const QString &a, const QString &b);

// Последний заголовок версии в <baseDir>/docs/changes.txt или пустая строка.
QString installed(const QString &baseDir);

// Принимает ТОЛЬКО https://github.com/bol-van/zapret2/releases/tag/vX.Y.Z, иначе пустая строка.
QString tagFromLocation(const QString &location);

QString releaseUrl(const QString &tag);

} // namespace ZapretVersion

class UpdateChecker : public QObject
{
    Q_OBJECT
public:
    explicit UpdateChecker(QObject *parent = nullptr);

    bool isRunning() const { return m_running; }
    void check();

signals:
    void finished(const QString &latestTag); // "v1.0.7"
    void failed(const QString &message);

private:
    QNetworkAccessManager *m_nam;
    bool m_running = false;
};
