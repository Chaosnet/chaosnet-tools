# Chaosnet Tools

### `gw` &mdash; Gateway incoming TCP connections to Chaosnet.

Usage: `gw` *port* *contact* *host*

Listen to TCP *port*, and forward the connection to the *contact*
service at *host*.  **WARNING** this may be dangerous since some
Chaosnet servers may not be hardened against malicious attacks.

There is a unit file chaosnet-gateway.service for systemd; make sure
to update `WorkingDirectory`.  Edit the gateway.sh script to your liking.

## `shutdown` &mdash; Request to shut down Chaosnet host.

Usage: `shutdown` *host* [*data*]

Send a request to *host* to shut itself down.  Optional *data* can be
sent which the host may interpret as information about how to shut down.
