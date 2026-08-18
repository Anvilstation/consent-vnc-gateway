#include "transport/GatewayAdaptor.h"

#include "application/AccessCoordinator.h"

namespace gateway {

GatewayAdaptor::GatewayAdaptor(AccessCoordinator &coordinator, QObject *parent)
    : QDBusAbstractAdaptor(parent), m_coordinator(coordinator)
{
}

int GatewayAdaptor::requestAccess(const QString &peerAddress, const QString &peerName)
{
    return m_coordinator.requestAccess(peerAddress, peerName);
}

int GatewayAdaptor::confirmConnected(const QString &peerAddress)
{
    return m_coordinator.confirmConnected(peerAddress);
}

int GatewayAdaptor::connectionRejected(const QString &peerAddress)
{
    return m_coordinator.connectionRejected(peerAddress);
}

int GatewayAdaptor::connectionClosed(const QString &peerAddress)
{
    return m_coordinator.connectionClosed(peerAddress);
}

} // namespace gateway
