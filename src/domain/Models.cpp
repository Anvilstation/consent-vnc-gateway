#include "domain/Models.h"

#include <QHostAddress>

namespace gateway {

QJsonObject HostIdentity::toJson() const
{
    return {{QStringLiteral("hostName"), hostName},
            {QStringLiteral("ipAddress"), ipAddress},
            {QStringLiteral("macAddress"), macAddress},
            {QStringLiteral("userName"), userName}};
}

bool ConnectionRequest::isValid() const
{
    QHostAddress address;
    return !peerAddress.trimmed().isEmpty() && address.setAddress(peerAddress.trimmed());
}

} // namespace gateway
