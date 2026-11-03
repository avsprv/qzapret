#pragma once

#include <QObject>
#include <QString>

// Единственная точка контакта с апстримом zapret2.
// При смене CLI/юнитов апстрима правится только реализация этого интерфейса.
class ZapretBackend : public QObject
{
    Q_OBJECT
public:
    enum class State { Unknown, Active, Inactive, Failed };
    Q_ENUM(State)

    using QObject::QObject;
    ~ZapretBackend() override = default;

    virtual void refresh() = 0; // асинхронно, результат придёт в stateChanged
    virtual void start() = 0;
    virtual void stop() = 0;
    // TODO: applyProfile(const Profile &) — запись конфига через привилегированный хелпер

signals:
    void stateChanged(ZapretBackend::State state);
    void errorOccurred(const QString &message);
};
