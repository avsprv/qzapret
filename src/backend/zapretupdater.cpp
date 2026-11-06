#include "zapretupdater.h"

#include <QProcess>
#include <QRegularExpression>

#include "helperpath.h"

ZapretUpdater::ZapretUpdater(QObject *parent)
    : QObject(parent)
{
}

void ZapretUpdater::start(const QString &tag)
{
    if (m_proc)
        return;
    static const QRegularExpression valid(QStringLiteral(R"(^v\d+(\.\d+)*$)"));
    if (!valid.match(tag).hasMatch()) {
        emit failedToStart(tr("Некорректная версия: %1").arg(tag));
        emit finished(-1);
        return;
    }

    m_buf.clear();
    m_proc = new QProcess(this);
    m_proc->setProcessChannelMode(QProcess::MergedChannels);
    connect(m_proc, &QProcess::readyReadStandardOutput, this, &ZapretUpdater::drain);
    connect(m_proc, &QProcess::finished, this, [this](int code, QProcess::ExitStatus st) {
        drain();
        if (!m_buf.isEmpty())
            emit logLine(QString::fromUtf8(m_buf));
        m_buf.clear();
        QProcess *p = m_proc;
        m_proc = nullptr;
        p->deleteLater();
        emit finished(st == QProcess::NormalExit ? code : -1);
    });
    connect(m_proc, &QProcess::errorOccurred, this, [this](QProcess::ProcessError err) {
        if (err != QProcess::FailedToStart)
            return;
        const QString msg = tr("Не удалось запустить pkexec/хелпер: %1").arg(m_proc->errorString());
        QProcess *p = m_proc;
        m_proc = nullptr;
        p->deleteLater();
        emit failedToStart(msg);
        emit finished(-1);
    });
    Helper::start(m_proc, {QStringLiteral("update"), tag});
}

void ZapretUpdater::drain()
{
    if (!m_proc)
        return;
    m_buf += m_proc->readAllStandardOutput();
    int nl;
    while ((nl = m_buf.indexOf('\n')) >= 0) {
        QString line = QString::fromUtf8(m_buf.left(nl));
        m_buf.remove(0, nl + 1);
        if (line.endsWith(u'\r'))
            line.chop(1);
        emit logLine(line);
    }
}
