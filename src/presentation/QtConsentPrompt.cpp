#include "presentation/QtConsentPrompt.h"

#include <QAbstractButton>
#include <QMessageBox>
#include <QTimer>

namespace gateway {

QtConsentPrompt::QtConsentPrompt(int timeoutSeconds)
    : m_timeoutSeconds(timeoutSeconds)
{
}

bool QtConsentPrompt::ask(const ConnectionRequest &request)
{
    const QString peer = request.peerName.isEmpty()
        ? request.peerAddress
        : QStringLiteral("%1 (%2)").arg(request.peerName, request.peerAddress);
    const QString baseText = QObject::tr(
        "A remote-support peer is requesting access to this desktop.<br><br>"
        "Peer: <b>%1</b><br><br>Allow the connection?").arg(peer.toHtmlEscaped());

    QMessageBox box(QMessageBox::Question,
                    QObject::tr("Remote access request"),
                    baseText,
                    QMessageBox::Yes | QMessageBox::No);
    box.setDefaultButton(QMessageBox::No);
    int remaining = m_timeoutSeconds;
    box.setText(baseText + QObject::tr("<br><br>Timeout: %1 seconds").arg(remaining));

    QTimer timer(&box);
    timer.setInterval(1000);
    QObject::connect(&timer, &QTimer::timeout, &box, [&] {
        --remaining;
        box.setText(baseText + QObject::tr("<br><br>Timeout: %1 seconds").arg(remaining));
        if (remaining <= 0) {
            timer.stop();
            if (auto *button = box.button(QMessageBox::No))
                button->click();
        }
    });
    timer.start();
    return box.exec() == QMessageBox::Yes;
}

void QtConsentPrompt::showCredential(const QString &credential)
{
    QMessageBox::information(nullptr,
                             QObject::tr("One-time credential"),
                             QObject::tr("Share this one-time VNC credential through an approved "
                                         "channel:<br><br><b>%1</b>")
                                 .arg(credential.toHtmlEscaped()));
}

} // namespace gateway
