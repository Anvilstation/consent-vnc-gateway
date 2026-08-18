#pragma once

#include <QString>

namespace gateway {

struct VncSettings
{
    bool dryRun = true;
    QString executable = QStringLiteral("x11vnc");
    QString passwordFile = QStringLiteral("/tmp/consent-vnc-gateway.pass");
    QString display = QStringLiteral(":0");
    int credentialDigits = 6;
};

struct AppConfig
{
    QString bus = QStringLiteral("session");
    QString service = QStringLiteral("io.github.consentvnc.Gateway");
    QString objectPath = QStringLiteral("/Gateway");
    QString sqlitePath;
    int consentTimeoutSeconds = 30;
    VncSettings vnc;

    static AppConfig load(const QString &path, QString *error);
};

} // namespace gateway
