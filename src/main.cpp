#include "application/AccessCoordinator.h"
#include "config/AppConfig.h"
#include "infrastructure/LocalHostIdentityProvider.h"
#include "infrastructure/SqliteSessionRepository.h"
#include "infrastructure/SystemVncController.h"
#include "presentation/QtConsentPrompt.h"
#include "transport/GatewayAdaptor.h"

#include <QApplication>
#include <QCommandLineParser>
#include <QDBusConnection>
#include <QDBusError>
#include <QDir>
#include <QMenu>
#include <QMessageBox>
#include <QStandardPaths>
#include <QStyle>
#include <QSystemTrayIcon>

namespace {

QDBusConnection selectedBus(const QString &name)
{
    return name == QStringLiteral("system") ? QDBusConnection::systemBus()
                                             : QDBusConnection::sessionBus();
}

} // namespace

int main(int argc, char *argv[])
{
    QApplication app(argc, argv);
    QApplication::setApplicationName(QStringLiteral("Consent VNC Gateway"));
    QApplication::setOrganizationName(QStringLiteral("Technology Demo"));
    QApplication::setQuitOnLastWindowClosed(false);

    QCommandLineParser parser;
    parser.setApplicationDescription(QStringLiteral("User-consent gateway for VNC sessions"));
    parser.addHelpOption();
    QCommandLineOption configOption(
        {QStringLiteral("c"), QStringLiteral("config")},
        QStringLiteral("Path to the JSON configuration."),
        QStringLiteral("file"),
        QDir(QCoreApplication::applicationDirPath())
            .filePath(QStringLiteral("config/access-gateway.example.json")));
    parser.addOption(configOption);
    parser.process(app);

    QString error;
    const auto config = gateway::AppConfig::load(parser.value(configOption), &error);
    if (!error.isEmpty()) {
        QMessageBox::critical(nullptr, QStringLiteral("Configuration error"), error);
        return 1;
    }

    QString sqlitePath = config.sqlitePath;
    if (sqlitePath.isEmpty()) {
        sqlitePath = QDir(QStandardPaths::writableLocation(QStandardPaths::AppDataLocation))
                         .filePath(QStringLiteral("sessions.sqlite3"));
    }

    gateway::SqliteSessionRepository sessions(sqlitePath);
    if (!sessions.initialize(&error)) {
        QMessageBox::critical(nullptr, QStringLiteral("Storage error"), error);
        return 2;
    }
    gateway::LocalHostIdentityProvider identity;
    gateway::SystemVncController vnc(config.vnc);
    gateway::QtConsentPrompt prompt(config.consentTimeoutSeconds);
    gateway::AccessCoordinator coordinator(prompt, sessions, vnc, identity);
    if (!coordinator.recover(&error))
        qWarning("Could not recover open sessions: %s", qPrintable(error));

    QObject dbusObject;
    gateway::GatewayAdaptor adaptor(coordinator, &dbusObject);
    auto bus = selectedBus(config.bus);
    if (!bus.registerService(config.service)
        || !bus.registerObject(config.objectPath, &dbusObject, QDBusConnection::ExportAdaptors)) {
        QMessageBox::critical(nullptr,
                              QStringLiteral("DBus registration error"),
                              bus.lastError().message());
        return 3;
    }

    QSystemTrayIcon tray(app.style()->standardIcon(QStyle::SP_ComputerIcon));
    QMenu menu;
    menu.addAction(QStringLiteral("Quit"), &app, &QApplication::quit);
    tray.setContextMenu(&menu);
    tray.setToolTip(QStringLiteral("Consent VNC Gateway"));
    tray.show();

    return app.exec();
}
