#pragma once

#include "domain/Models.h"

namespace gateway {

class ConsentPrompt
{
public:
    virtual ~ConsentPrompt() = default;
    virtual bool ask(const ConnectionRequest &request) = 0;
    virtual void showCredential(const QString &credential) = 0;
};

class SessionRepository
{
public:
    virtual ~SessionRepository() = default;
    virtual bool initialize(QString *error) = 0;
    virtual bool create(const AccessSession &session, QString *error) = 0;
    virtual bool markDisconnected(const QString &sessionId,
                                  const QDateTime &when,
                                  QString *error) = 0;
    virtual int recoverOpenSessions(const HostIdentity &host,
                                    const QDateTime &when,
                                    QString *error) = 0;
};

class VncController
{
public:
    virtual ~VncController() = default;
    virtual QString rotateCredential(QString *error) = 0;
    virtual bool disconnectAll(QString *error) = 0;
};

class HostIdentityProvider
{
public:
    virtual ~HostIdentityProvider() = default;
    virtual HostIdentity current() const = 0;
};

} // namespace gateway
