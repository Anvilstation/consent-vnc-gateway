# Consent VNC Gateway

A neutral Qt/DBus reference implementation for user-approved VNC access. The
gateway separates consent, session state, VNC process control, and audit
storage. It contains no organization-specific identities, infrastructure,
credentials, certificates, logos, or database endpoints.

**Status: experimental reference implementation, not a production access-control
boundary.** The event hooks and x11vnc deployment must be integrated and secured
separately; the defaults only simulate credential changes.

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
3. On acceptance, the gateway rotates the shared VNC credential and displays it
   to the user.
4. A separate event from a trusted integration hook reports that the VNC session
   started; only then is an audit record created.
5. The closing event timestamps the audit record and rotates the credential.

This two-stage model avoids recording unconfirmed attempts as completed sessions.
Only one peer may be pending or active at a time because x11vnc uses one shared
password file.

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
ctest --test-dir build --output-on-failure
./build/consent-vnc-gateway --config config/access-gateway.example.json
```

Tests are enabled by default and additionally require the Qt Test component.
Use `-DBUILD_TESTING=OFF` when configuring a build without tests.

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

Return values are `1` for approval/confirmation/completed closure (including
duplicate approvals), `0` for denial or an ignored event, `-1` for an invalid
request address, and `-2` for a storage or credential error. Closing a pending
request invalidates it without creating an audit record and returns `0`.
If closing an active session returns `-2`, the integration can retry the event.

## Integration limits

- Peer labels and addresses are supplied by the caller, not independently
  authenticated. DBus access must be restricted to trusted local hooks; the
  gateway does not bind a VNC authentication attempt to the approved peer.
- The consent timeout limits the approval dialog only. Pending approvals do not
  expire automatically, and the shared credential is not a single-use token.
  Rejection/closure events must be delivered reliably by the integration.
- Credential rotation alone does not disconnect an established VNC client.
  Crash recovery closes audit records; it does not revoke live VNC sessions.
- The example calls x11vnc with the generated password as a process argument and
  uses a predictable path under `/tmp`. A production helper must prevent process
  argument disclosure, unsafe file replacement, and unauthorized file access.
- Classic VNC password authentication has protocol limitations, including an
  eight-byte password limit. The demo's numeric credential is not sufficient for
  an Internet-facing deployment. Use a protected transport and review the VNC
  server's authentication and rate-limiting settings.

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
