#pragma once

#include "application/Interfaces.h"

#include <QHash>
#include <QObject>

namespace gateway {

class AccessCoordinator final : public QObject
{
    Q_OBJECT
public:
    AccessCoordinator(ConsentPrompt &prompt,
                      SessionRepository &sessions,
                      VncController &vnc,
                      HostIdentityProvider &identity,
                      QObject *parent = nullptr);

    bool recover(QString *error);
    int requestAccess(const QString &peerAddress, const QString &peerName);
    int confirmConnected(const QString &peerAddress);
    int connectionRejected(const QString &peerAddress);
    int connectionClosed(const QString &peerAddress);

private:
    ConsentPrompt &m_prompt;
    SessionRepository &m_sessions;
    VncController &m_vnc;
    HostIdentityProvider &m_identity;
    QHash<QString, ConnectionRequest> m_pending;
    QHash<QString, QString> m_activeSessionIds;
};

} // namespace gateway
