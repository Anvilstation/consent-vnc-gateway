#include "application/AccessCoordinator.h"

#include <QDebug>
#include <QScopedValueRollback>
#include <QUuid>

namespace gateway {

AccessCoordinator::AccessCoordinator(ConsentPrompt &prompt,
                                     SessionRepository &sessions,
                                     VncController &vnc,
                                     HostIdentityProvider &identity,
                                     QObject *parent)
    : QObject(parent),
      m_prompt(prompt),
      m_sessions(sessions),
      m_vnc(vnc),
      m_identity(identity)
{
}

bool AccessCoordinator::recover(QString *error)
{
    return m_sessions.recoverOpenSessions(
               m_identity.current(), QDateTime::currentDateTimeUtc(), error)
        >= 0;
}

int AccessCoordinator::requestAccess(const QString &peerAddress, const QString &peerName)
{
    const QString normalizedAddress = peerAddress.trimmed();
    ConnectionRequest request{normalizedAddress, peerName.trimmed(), QDateTime::currentDateTimeUtc()};
    if (!request.isValid())
        return -1;
    if (m_activeSessionIds.contains(normalizedAddress) || m_pending.contains(normalizedAddress))
        return 1;
    // x11vnc has one shared credential: a second approval would invalidate the first.
    // The guard also covers the nested event loop used by the modal consent prompt.
    if (m_promptOpen || !m_pending.isEmpty() || !m_activeSessionIds.isEmpty())
        return 0;
    QScopedValueRollback<bool> promptOpen(m_promptOpen, true);
    if (!m_prompt.ask(request))
        return 0;

    QString error;
    const QString credential = m_vnc.rotateCredential(&error);
    if (credential.isEmpty()) {
        qWarning() << "Could not rotate VNC credential:" << error;
        return -2;
    }
    m_pending.insert(normalizedAddress, request);
    m_prompt.showCredential(credential);
    return 1;
}

int AccessCoordinator::confirmConnected(const QString &peerAddress)
{
    const QString normalizedAddress = peerAddress.trimmed();
    if (m_activeSessionIds.contains(normalizedAddress))
        return 1;
    if (!m_pending.contains(normalizedAddress))
        return 0;

    AccessSession session;
    session.id = QUuid::createUuid().toString(QUuid::WithoutBraces);
    session.host = m_identity.current();
    session.request = m_pending.value(normalizedAddress);
    session.connectedAt = QDateTime::currentDateTimeUtc();

    QString error;
    if (!m_sessions.create(session, &error)) {
        qWarning() << "Could not write session:" << error;
        return -2;
    }
    m_pending.remove(normalizedAddress);
    m_activeSessionIds.insert(normalizedAddress, session.id);
    return 1;
}

int AccessCoordinator::connectionRejected(const QString &peerAddress)
{
    const QString normalizedAddress = peerAddress.trimmed();
    if (!m_pending.contains(normalizedAddress))
        return 0;
    m_pending.remove(normalizedAddress);
    QString error;
    if (m_vnc.rotateCredential(&error).isEmpty()) {
        qWarning() << "Could not invalidate VNC credential:" << error;
        return -2;
    }
    return 0;
}

int AccessCoordinator::connectionClosed(const QString &peerAddress)
{
    const QString normalizedAddress = peerAddress.trimmed();
    const QString sessionId = m_activeSessionIds.value(normalizedAddress);
    if (sessionId.isEmpty())
        return connectionRejected(normalizedAddress);

    QString error;
    const bool recorded = m_sessions.markDisconnected(
        sessionId, QDateTime::currentDateTimeUtc(), &error);
    if (!recorded)
        qWarning() << "Could not close session:" << error;
    const bool invalidated = !m_vnc.rotateCredential(&error).isEmpty();
    if (!invalidated)
        qWarning() << "Could not rotate VNC credential after close:" << error;
    if (!recorded || !invalidated)
        return -2;
    m_activeSessionIds.remove(normalizedAddress);
    return 1;
}

} // namespace gateway
