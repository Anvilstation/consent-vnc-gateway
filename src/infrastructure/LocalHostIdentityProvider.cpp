#include "infrastructure/LocalHostIdentityProvider.h"

#include <QHostInfo>
#include <QNetworkInterface>

namespace gateway {

HostIdentity LocalHostIdentityProvider::current() const
{
    HostIdentity result;
    result.hostName = QHostInfo::localHostName();
    result.userName = qEnvironmentVariable("USER", qEnvironmentVariable("USERNAME"));

    for (const auto &interface : QNetworkInterface::allInterfaces()) {
        const auto flags = interface.flags();
        if (!flags.testFlag(QNetworkInterface::IsUp)
            || flags.testFlag(QNetworkInterface::IsLoopBack)) {
            continue;
        }
        for (const auto &entry : interface.addressEntries()) {
            if (entry.ip().protocol() == QAbstractSocket::IPv4Protocol) {
                result.ipAddress = entry.ip().toString();
                result.macAddress = interface.hardwareAddress();
                return result;
            }
        }
    }
    return result;
}

} // namespace gateway
