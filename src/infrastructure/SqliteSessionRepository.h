#pragma once

#include "application/Interfaces.h"

#include <QSqlDatabase>

namespace gateway {

class SqliteSessionRepository final : public SessionRepository
{
public:
    explicit SqliteSessionRepository(QString databasePath);
    ~SqliteSessionRepository() override;

    bool initialize(QString *error) override;
    bool create(const AccessSession &session, QString *error) override;
    bool markDisconnected(const QString &sessionId,
                          const QDateTime &when,
                          QString *error) override;
    int recoverOpenSessions(const HostIdentity &host,
                            const QDateTime &when,
                            QString *error) override;

private:
    bool open(QString *error);

    QString m_databasePath;
    QString m_connectionName;
    QSqlDatabase m_database;
};

} // namespace gateway
