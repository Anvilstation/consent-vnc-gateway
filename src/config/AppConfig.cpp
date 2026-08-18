#include "config/AppConfig.h"

#include <QFile>
#include <QJsonDocument>
#include <QJsonObject>

namespace gateway {

AppConfig AppConfig::load(const QString &path, QString *error)
{
    AppConfig config;
    QFile file(path);
    if (!file.open(QIODevice::ReadOnly)) {
        if (error)
            *error = file.errorString();
        return config;
    }

    QJsonParseError parseError;
    const auto document = QJsonDocument::fromJson(file.readAll(), &parseError);
    if (parseError.error != QJsonParseError::NoError || !document.isObject()) {
        if (error)
            *error = parseError.errorString();
        return config;
    }

    const auto root = document.object();
    const auto dbus = root.value(QStringLiteral("dbus")).toObject();
    const auto storage = root.value(QStringLiteral("storage")).toObject();
    const auto vnc = root.value(QStringLiteral("vnc")).toObject();

    config.bus = dbus.value(QStringLiteral("bus")).toString(config.bus);
    config.service = dbus.value(QStringLiteral("service")).toString(config.service);
    config.objectPath = dbus.value(QStringLiteral("objectPath")).toString(config.objectPath);
    config.sqlitePath = storage.value(QStringLiteral("sqlitePath")).toString();
    config.consentTimeoutSeconds = qBound(
        5, root.value(QStringLiteral("consentTimeoutSeconds")).toInt(30), 300);
    config.vnc.dryRun = vnc.value(QStringLiteral("dryRun")).toBool(true);
    config.vnc.executable = vnc.value(QStringLiteral("executable")).toString(config.vnc.executable);
    config.vnc.passwordFile =
        vnc.value(QStringLiteral("passwordFile")).toString(config.vnc.passwordFile);
    config.vnc.display = vnc.value(QStringLiteral("display")).toString(config.vnc.display);
    config.vnc.credentialDigits = qBound(
        4, vnc.value(QStringLiteral("credentialDigits")).toInt(6), 10);

    if (config.bus != QStringLiteral("session") && config.bus != QStringLiteral("system")) {
        if (error)
            *error = QStringLiteral("dbus.bus must be 'session' or 'system'.");
    }
    return config;
}

} // namespace gateway
