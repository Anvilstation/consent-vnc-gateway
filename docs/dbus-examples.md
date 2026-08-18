# DBus integration examples

The example configuration uses the session bus. These calls show the event
contract; replace the fictional addresses and labels in your local test.

```bash
qdbus io.github.consentvnc.Gateway /Gateway \
  io.github.consentvnc.Gateway.requestAccess \
  "192.0.2.10" "Example support operator"

qdbus io.github.consentvnc.Gateway /Gateway \
  io.github.consentvnc.Gateway.confirmConnected "192.0.2.10"

qdbus io.github.consentvnc.Gateway /Gateway \
  io.github.consentvnc.Gateway.connectionClosed "192.0.2.10"
```

`192.0.2.0/24` is reserved for documentation and is not a production endpoint.
