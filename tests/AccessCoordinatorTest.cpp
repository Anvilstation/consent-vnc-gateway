#include "application/AccessCoordinator.h"

#include <QtTest>

#include <functional>

namespace gateway {

class FakePrompt final : public ConsentPrompt
{
public:
    bool decision = true;
    int askCount = 0;
    QString shownCredential;
    std::function<void()> duringAsk;

    bool ask(const ConnectionRequest &) override
    {
        ++askCount;
        if (duringAsk)
            duringAsk();
        return decision;
    }

    void showCredential(const QString &credential) override
    {
        shownCredential = credential;
    }
};

class FakeRepository final : public SessionRepository
{
public:
    QList<AccessSession> created;
    QString disconnectedId;
    bool failClose = false;

    bool initialize(QString *) override { return true; }

    bool create(const AccessSession &session, QString *) override
    {
        created.append(session);
        return true;
    }

    bool markDisconnected(const QString &sessionId, const QDateTime &, QString *error) override
    {
        if (failClose) {
            if (error)
                *error = QStringLiteral("Simulated storage failure");
            return false;
        }
        disconnectedId = sessionId;
        return true;
    }

    int recoverOpenSessions(const HostIdentity &, const QDateTime &, QString *) override
    {
        return 0;
    }
};

class FakeVncController final : public VncController
{
public:
    int rotations = 0;
    bool failRotation = false;

    QString rotateCredential(QString *error) override
    {
        ++rotations;
        if (failRotation) {
            if (error)
                *error = QStringLiteral("Simulated credential rotation failure");
            return {};
        }
        return QStringLiteral("654321");
    }

    bool disconnectAll(QString *) override { return true; }
};

class FakeIdentityProvider final : public HostIdentityProvider
{
public:
    HostIdentity current() const override
    {
        return {QStringLiteral("demo-host"),
                QStringLiteral("192.0.2.20"),
                QStringLiteral("00:00:5E:00:53:00"),
                QStringLiteral("demo-user")};
    }
};

class AccessCoordinatorTest final : public QObject
{
    Q_OBJECT

private slots:
    void rejectsInvalidPeerAddress()
    {
        FakePrompt prompt;
        FakeRepository repository;
        FakeVncController vnc;
        FakeIdentityProvider identity;
        AccessCoordinator coordinator(prompt, repository, vnc, identity);

        QCOMPARE(coordinator.requestAccess(QStringLiteral("not-an-address"), QString()), -1);
        QCOMPARE(prompt.askCount, 0);
    }

    void rejectionDoesNotCreateSession()
    {
        FakePrompt prompt;
        prompt.decision = false;
        FakeRepository repository;
        FakeVncController vnc;
        FakeIdentityProvider identity;
        AccessCoordinator coordinator(prompt, repository, vnc, identity);

        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.10"), QString()), 0);
        QCOMPARE(repository.created.size(), 0);
        QCOMPARE(vnc.rotations, 0);
    }

    void recordsOnlyConfirmedConnectionAndClosesIt()
    {
        FakePrompt prompt;
        FakeRepository repository;
        FakeVncController vnc;
        FakeIdentityProvider identity;
        AccessCoordinator coordinator(prompt, repository, vnc, identity);

        QCOMPARE(coordinator.requestAccess(
                     QStringLiteral("192.0.2.10"), QStringLiteral("Example operator")),
                 1);
        QCOMPARE(prompt.shownCredential, QStringLiteral("654321"));
        QCOMPARE(repository.created.size(), 0);

        QCOMPARE(coordinator.confirmConnected(QStringLiteral("192.0.2.10")), 1);
        QCOMPARE(repository.created.size(), 1);
        QVERIFY(!repository.created.first().id.isEmpty());

        QCOMPARE(coordinator.connectionClosed(QStringLiteral("192.0.2.10")), 1);
        QCOMPARE(repository.disconnectedId, repository.created.first().id);
        QCOMPARE(vnc.rotations, 2);
    }

    void duplicatePendingRequestIsIdempotent()
    {
        FakePrompt prompt;
        FakeRepository repository;
        FakeVncController vnc;
        FakeIdentityProvider identity;
        AccessCoordinator coordinator(prompt, repository, vnc, identity);

        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.10"), QString()), 1);
        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.10"), QString()), 1);
        QCOMPARE(prompt.askCount, 1);
        QCOMPARE(vnc.rotations, 1);
    }

    void unknownEventsDoNotRotateSharedCredential()
    {
        FakePrompt prompt;
        FakeRepository repository;
        FakeVncController vnc;
        FakeIdentityProvider identity;
        AccessCoordinator coordinator(prompt, repository, vnc, identity);

        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.10"), QString()), 1);
        QCOMPARE(coordinator.connectionRejected(QStringLiteral("192.0.2.11")), 0);
        QCOMPARE(coordinator.connectionClosed(QStringLiteral("192.0.2.11")), 0);
        QCOMPARE(vnc.rotations, 1);
        QCOMPARE(coordinator.confirmConnected(QStringLiteral("192.0.2.10")), 1);
        QCOMPARE(coordinator.connectionRejected(QStringLiteral("192.0.2.10")), 0);
        QCOMPARE(vnc.rotations, 1);
    }

    void closingPendingRequestInvalidatesCredential()
    {
        FakePrompt prompt;
        FakeRepository repository;
        FakeVncController vnc;
        FakeIdentityProvider identity;
        AccessCoordinator coordinator(prompt, repository, vnc, identity);

        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.10"), QString()), 1);
        QCOMPARE(coordinator.connectionClosed(QStringLiteral("192.0.2.10")), 0);
        QCOMPARE(vnc.rotations, 2);
        QCOMPARE(coordinator.confirmConnected(QStringLiteral("192.0.2.10")), 0);
        QVERIFY(repository.created.isEmpty());
    }

    void failedCloseCanBeRetried()
    {
        FakePrompt prompt;
        FakeRepository repository;
        FakeVncController vnc;
        FakeIdentityProvider identity;
        AccessCoordinator coordinator(prompt, repository, vnc, identity);

        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.10"), QString()), 1);
        QCOMPARE(coordinator.confirmConnected(QStringLiteral("192.0.2.10")), 1);
        repository.failClose = true;
        QCOMPARE(coordinator.connectionClosed(QStringLiteral("192.0.2.10")), -2);
        QCOMPARE(vnc.rotations, 2);
        repository.failClose = false;
        QCOMPARE(coordinator.connectionClosed(QStringLiteral("192.0.2.10")), 1);
        QCOMPARE(repository.disconnectedId, repository.created.first().id);
    }

    void failedCloseRotationCanBeRetried()
    {
        FakePrompt prompt;
        FakeRepository repository;
        FakeVncController vnc;
        FakeIdentityProvider identity;
        AccessCoordinator coordinator(prompt, repository, vnc, identity);

        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.10"), QString()), 1);
        QCOMPARE(coordinator.confirmConnected(QStringLiteral("192.0.2.10")), 1);
        vnc.failRotation = true;
        QCOMPARE(coordinator.connectionClosed(QStringLiteral("192.0.2.10")), -2);
        vnc.failRotation = false;
        QCOMPARE(coordinator.connectionClosed(QStringLiteral("192.0.2.10")), 1);
        QCOMPARE(vnc.rotations, 3);
    }

    void secondPeerCannotReplaceSharedCredential()
    {
        FakePrompt prompt;
        FakeRepository repository;
        FakeVncController vnc;
        FakeIdentityProvider identity;
        AccessCoordinator coordinator(prompt, repository, vnc, identity);

        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.10"), QString()), 1);
        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.11"), QString()), 0);
        QCOMPARE(coordinator.confirmConnected(QStringLiteral("192.0.2.10")), 1);
        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.11"), QString()), 0);
        QCOMPARE(vnc.rotations, 1);
        QCOMPARE(prompt.askCount, 1);
        QCOMPARE(coordinator.connectionClosed(QStringLiteral("192.0.2.10")), 1);
        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.11"), QString()), 1);
    }

    void nestedRequestCannotOpenAnotherConsentPrompt()
    {
        FakePrompt prompt;
        FakeRepository repository;
        FakeVncController vnc;
        FakeIdentityProvider identity;
        AccessCoordinator coordinator(prompt, repository, vnc, identity);

        prompt.duringAsk = [&] {
            QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.10"), QString()), 0);
            QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.11"), QString()), 0);
        };
        QCOMPARE(coordinator.requestAccess(QStringLiteral("192.0.2.10"), QString()), 1);
        QCOMPARE(prompt.askCount, 1);
        QCOMPARE(vnc.rotations, 1);
    }
};

} // namespace gateway

QTEST_APPLESS_MAIN(gateway::AccessCoordinatorTest)

#include "AccessCoordinatorTest.moc"
