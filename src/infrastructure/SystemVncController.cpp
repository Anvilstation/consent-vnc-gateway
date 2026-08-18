#include "infrastructure/SystemVncController.h"

#include <QProcess>
#include <QRandomGenerator>

#include <utility>

namespace gateway {

SystemVncController::SystemVncController(VncSettings settings)
    : m_settings(std::move(settings))
{
}

QString SystemVncController::rotateCredential(QString *error)
{
    QString credential;
    credential.reserve(m_settings.credentialDigits);
    for (int i = 0; i < m_settings.credentialDigits; ++i)
        credential.append(QString::number(QRandomGenerator::system()->bounded(10)));

    if (!m_settings.dryRun
        && !execute({QStringLiteral("-storepasswd"), credential, m_settings.passwordFile}, error)) {
        return {};
    }
    return credential;
}

bool SystemVncController::disconnectAll(QString *error)
{
    if (m_settings.dryRun)
        return true;
    return execute({QStringLiteral("-display"), m_settings.display,
                    QStringLiteral("-remote"), QStringLiteral("disconnect:all")},
                   error);
}

bool SystemVncController::execute(const QStringList &arguments, QString *error) const
{
    QProcess process;
    process.start(m_settings.executable, arguments);
    if (!process.waitForStarted(3000) || !process.waitForFinished(10000)
        || process.exitStatus() != QProcess::NormalExit || process.exitCode() != 0) {
        if (error) {
            const QString stderrText = QString::fromUtf8(process.readAllStandardError()).trimmed();
            *error = stderrText.isEmpty() ? process.errorString() : stderrText;
        }
        return false;
    }
    return true;
}

} // namespace gateway
