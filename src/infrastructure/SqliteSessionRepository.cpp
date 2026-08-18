#include "infrastructure/SqliteSessionRepository.h"

#include <QDir>
#include <QFileInfo>
#include <QSqlError>
#include <QSqlQuery>
#include <QUuid>

#include <utility>

namespace gateway {

SqliteSessionRepository::SqliteSessionRepository(QString databasePath)
    : m_databasePath(std::move(databasePath)),
      m_connectionName(QStringLiteral("consent_gateway_%1")
                           .arg(QUuid::createUuid().toString(QUuid::WithoutBraces)))
{
}

SqliteSessionRepository::~SqliteSessionRepository()
{
    if (m_database.isOpen())
        m_database.close();
    m_database = QSqlDatabase();
    QSqlDatabase::removeDatabase(m_connectionName);
}

bool SqliteSessionRepository::open(QString *error)
{
    if (m_database.isOpen())
        return true;
    if (!QDir().mkpath(QFileInfo(m_databasePath).absolutePath())) {
        if (error)
            *error = QStringLiteral("Could not create the database directory.");
        return false;
    }
    m_database = QSqlDatabase::addDatabase(QStringLiteral("QSQLITE"), m_connectionName);
    m_database.setDatabaseName(m_databasePath);
    if (!m_database.open()) {
        if (error)
            *error = m_database.lastError().text();
        return false;
    }
    return true;
}

bool SqliteSessionRepository::initialize(QString *error)
{
    if (!open(error))
        return false;
    QSqlQuery query(m_database);
    const bool ok = query.exec(QStringLiteral(
        "CREATE TABLE IF NOT EXISTS access_sessions ("
        "session_id TEXT PRIMARY KEY, host_name TEXT, host_ip TEXT, host_mac TEXT, "
        "host_user TEXT, peer_address TEXT NOT NULL, peer_name TEXT, requested_at TEXT NOT NULL, "
        "connected_at TEXT NOT NULL, disconnected_at TEXT)"));
    if (!ok && error)
        *error = query.lastError().text();
    return ok;
}

bool SqliteSessionRepository::create(const AccessSession &session, QString *error)
{
    if (!open(error))
        return false;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "INSERT INTO access_sessions (session_id, host_name, host_ip, host_mac, host_user, "
        "peer_address, peer_name, requested_at, connected_at) "
        "VALUES (:id, :host_name, :host_ip, :host_mac, :host_user, :peer_address, :peer_name, "
        ":requested_at, :connected_at)"));
    query.bindValue(QStringLiteral(":id"), session.id);
    query.bindValue(QStringLiteral(":host_name"), session.host.hostName);
    query.bindValue(QStringLiteral(":host_ip"), session.host.ipAddress);
    query.bindValue(QStringLiteral(":host_mac"), session.host.macAddress);
    query.bindValue(QStringLiteral(":host_user"), session.host.userName);
    query.bindValue(QStringLiteral(":peer_address"), session.request.peerAddress);
    query.bindValue(QStringLiteral(":peer_name"), session.request.peerName);
    query.bindValue(QStringLiteral(":requested_at"), session.request.requestedAt.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":connected_at"), session.connectedAt.toString(Qt::ISODate));
    const bool ok = query.exec();
    if (!ok && error)
        *error = query.lastError().text();
    return ok;
}

bool SqliteSessionRepository::markDisconnected(const QString &sessionId,
                                                const QDateTime &when,
                                                QString *error)
{
    if (!open(error))
        return false;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "UPDATE access_sessions SET disconnected_at = :when WHERE session_id = :id"));
    query.bindValue(QStringLiteral(":when"), when.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":id"), sessionId);
    const bool ok = query.exec();
    if (!ok && error)
        *error = query.lastError().text();
    return ok;
}

int SqliteSessionRepository::recoverOpenSessions(const HostIdentity &host,
                                                 const QDateTime &when,
                                                 QString *error)
{
    if (!open(error))
        return -1;
    QSqlQuery query(m_database);
    query.prepare(QStringLiteral(
        "UPDATE access_sessions SET disconnected_at = :when "
        "WHERE disconnected_at IS NULL AND host_name = :host_name"));
    query.bindValue(QStringLiteral(":when"), when.toString(Qt::ISODate));
    query.bindValue(QStringLiteral(":host_name"), host.hostName);
    if (!query.exec()) {
        if (error)
            *error = query.lastError().text();
        return -1;
    }
    return query.numRowsAffected();
}

} // namespace gateway
