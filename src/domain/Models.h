#pragma once

#include <QDateTime>
#include <QJsonObject>
#include <QString>

namespace gateway {

struct HostIdentity
{
    QString hostName;
    QString ipAddress;
    QString macAddress;
    QString userName;

    QJsonObject toJson() const;
};

struct ConnectionRequest
{
    QString peerAddress;
    QString peerName;
    QDateTime requestedAt;

    bool isValid() const;
};

struct AccessSession
{
    QString id;
    HostIdentity host;
    ConnectionRequest request;
    QDateTime connectedAt;
    QDateTime disconnectedAt;
};

} // namespace gateway
