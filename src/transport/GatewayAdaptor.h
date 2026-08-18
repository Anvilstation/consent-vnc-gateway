#pragma once

#include <QDBusAbstractAdaptor>

namespace gateway {

class AccessCoordinator;

class GatewayAdaptor final : public QDBusAbstractAdaptor
{
    Q_OBJECT
    Q_CLASSINFO("D-Bus Interface", "io.github.consentvnc.Gateway")

public:
    GatewayAdaptor(AccessCoordinator &coordinator, QObject *parent);

public slots:
    int requestAccess(const QString &peerAddress, const QString &peerName);
    int confirmConnected(const QString &peerAddress);
    int connectionRejected(const QString &peerAddress);
    int connectionClosed(const QString &peerAddress);

private:
    AccessCoordinator &m_coordinator;
};

} // namespace gateway
