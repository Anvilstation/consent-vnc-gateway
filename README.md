# Consent VNC Gateway

A neutral Qt/DBus reference implementation for user-approved VNC access. The
gateway separates consent, session state, VNC process control, and audit
storage. It contains no organization-specific identities, infrastructure,
credentials, certificates, logos, or database endpoints.

## Flow

```text
VNC event hook
    -> DBus GatewayAdaptor
    -> AccessCoordinator
       -> QtConsentPrompt
       -> SystemVncController
       -> SqliteSessionRepository
```

1. A peer requests access through DBus.
2. The desktop user sees the peer label/address and explicitly accepts or
   rejects the request.
3. On acceptance, the gateway rotates a one-time VNC credential and displays it
   to the user.
4. A separate connection event confirms that the VNC session really started;
   only then is an audit record created.
5. The closing event timestamps the audit record and rotates the credential.

This two-stage model avoids recording failed connection attempts as completed
sessions.

## Safe defaults

The example configuration uses the session bus, a local SQLite database, and
`dryRun: true`. In dry-run mode no x11vnc command is executed. Change these
settings only after reviewing the DBus policy and the x11vnc integration on a
test machine.

## Build

Requirements: Linux, CMake 3.16+, a C++17 compiler, and Qt 5 or Qt 6 with
Widgets, Network, DBus, and SQL (SQLite driver).

```bash
cmake -S . -B build
cmake --build build
./build/consent-vnc-gateway --config config/access-gateway.example.json
```

## DBus contract

Default service: `io.github.consentvnc.Gateway`

Default object path: `/Gateway`

Methods exposed by the `io.github.consentvnc.Gateway` interface:

- `requestAccess(peerAddress, peerName)` — asks the desktop user for consent.
- `confirmConnected(peerAddress)` — records a genuinely established session.
- `connectionRejected(peerAddress)` — invalidates a previously approved request.
- `connectionClosed(peerAddress)` — closes the matching audit session.

Example calls are in `docs/dbus-examples.md`. A restrictive system-bus policy
template is in `packaging/dbus`; it intentionally contains a placeholder group.

## Production checklist

- Run the gateway as an unprivileged desktop process.
- Give only a narrowly scoped helper permission to update the password file or
  signal x11vnc; do not run the whole GUI as root.
- Replace the placeholder DBus group and install a deny-by-default policy.
- Protect the SQLite audit database with filesystem permissions or replace the
  `SessionRepository` implementation with an approved backend.
- Decide which host/peer attributes may be retained under your privacy policy.
- Test timeout, rejection, reconnect, crash recovery, and multi-peer cases.

No open-source license has been selected yet.
