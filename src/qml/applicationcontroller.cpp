#include "applicationcontroller.h"

#include "backend/systemdbackend.h"

ApplicationController::ApplicationController(QObject *parent)
    : QObject(parent)
    , m_backend(new SystemdBackend(this))
{
    connect(m_backend, &ZapretBackend::stateChanged,
            this, &ApplicationController::setState);
    connect(m_backend, &ZapretBackend::bootEnabledChanged, this, [this](bool enabled) {
        if (m_bootEnabled == enabled)
            return;
        m_bootEnabled = enabled;
        emit bootEnabledChanged();
    });
    connect(m_backend, &ZapretBackend::errorOccurred, this, [this](const QString &message) {
        if (m_errorMessage == message)
            return;
        m_errorMessage = message;
        emit errorMessageChanged();
    });
    connect(m_backend, &ZapretBackend::unitActionFinished, this,
            [this](const QString &, ZapretBackend::State state, bool) {
        setState(state);
    });
    refresh();
}

QString ApplicationController::serviceState() const
{
    switch (m_state) {
    case ZapretBackend::State::Active:
        return tr("Работает");
    case ZapretBackend::State::Inactive:
        return tr("Остановлен");
    case ZapretBackend::State::Failed:
        return tr("Ошибка");
    case ZapretBackend::State::Unknown:
    default:
        return tr("Неизвестно");
    }
}

bool ApplicationController::serviceActive() const
{
    return m_state == ZapretBackend::State::Active;
}

QString ApplicationController::serviceStateCode() const
{
    switch (m_state) {
    case ZapretBackend::State::Active: return QStringLiteral("active");
    case ZapretBackend::State::Inactive: return QStringLiteral("inactive");
    case ZapretBackend::State::Failed: return QStringLiteral("failed");
    case ZapretBackend::State::Unknown:
    default: return QStringLiteral("unknown");
    }
}

bool ApplicationController::bootEnabled() const
{
    return m_bootEnabled;
}

bool ApplicationController::operationsBusy() const
{
    return m_operationsBusy;
}

void ApplicationController::setOperationsBusy(bool busy)
{
    if (m_operationsBusy == busy)
        return;
    m_operationsBusy = busy;
    emit operationsBusyChanged();
}

QString ApplicationController::errorMessage() const
{
    return m_errorMessage;
}

void ApplicationController::refresh()
{
    m_backend->refresh();
}

void ApplicationController::startService()
{
    m_errorMessage.clear();
    emit errorMessageChanged();
    m_backend->start();
}

void ApplicationController::stopService()
{
    m_errorMessage.clear();
    emit errorMessageChanged();
    m_backend->stop();
}

void ApplicationController::setBootEnabled(bool enabled)
{
    m_backend->setBootEnabled(enabled);
}

void ApplicationController::setState(ZapretBackend::State state)
{
    if (m_state == state)
        return;
    m_state = state;
    emit serviceStateChanged();
}
