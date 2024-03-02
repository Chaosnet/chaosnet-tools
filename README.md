# Chaosnet Tools

## `gw` &mdash; Gateway incoming TCP connections to Chaosnet.

Usage: `gw` *port* *contact* *host*

Listen to TCP *port*, and forward the connection to the *contact*
service at *host*.  **WARNING** this may be dangerous since some
Chaosnet servers may not be hardened against malicious attacks.

There is a unit file chaosnet-gateway.service for systemd; make sure
to update `WorkingDirectory`.  Edit the gateway.sh script to your liking.

## `mlftp` &mdash; File transfer using the MLDEV protocol.

Usage: `mlftp` `-r|w` *host* *ITS-file* *local-file*

To read a file from ITS, use `-r`; to write, use `-w`.  *ITS-file*
uses the conventional free-form syntax.  The device, directory, and
second file name are all optional.  They default to `DSK`, `.TEMP.`,
and `>`, respectively.

Since ITS file names can contain characters that confuse a typial Unix
shell, it's best to put `--` before it and surround it with quotes.
For example:

`mlftp -w its -- "-READ- -THIS-" readme.txt`

## `rtape` &mdash; Server for RTAPE remote tape protocol.

Usage: `rtape` `[-adqrv]`

`rtape` is a Unix program that implements a server for the RTAPE
protocol, which provides remote access to a tape drive.

This program was implemented primarily by reading and testing against
the ITS **DUMP** tape backup program, which can optionally use RTAPE.
The Unix **rtape** server from MIT/Symbolics was used as a secondary
reference.

#### Tape data format

When a client connects, it must specify which tape drive to use.  This
server uses the drive name to open a file.  Data read from or written
to this file will be stored in the SIMH tape image format.

#### Options

```
  -a  Allow slashes in mount drive name.
  -d  Run as daemon.
  -q  Quiet operation - no logging, just errors.
  -r  Only allow read-only mounts.
  -v  Verbose operation - detailed logging.
```

The `-a` option is dangerous.  The default is to not allow slashes, to
avoid people poking around the host by sending a drive name like
`/etc/password` or `../foobar`.

The default is to allow writing tapes, but `-r` is available for
cautious people.

#### Example

For example, if the rtape server is running on the host 177001, the
ITS **DUMP** backup program can mount a remote tape like this:

```
*dump^K
DUMP  .448

_remote
TAPE SERVER HOST=177001
DRIVE=backup.tap
READ ONLY? n
REMOTE TAPE UNWOND
_
```

## `shutdown` &mdash; Request to shut down Chaosnet host.

Usage: `shutdown` *host* [*data*]

Send a request to *host* to shut itself down.  Optional *data* can be
sent which the host may interpret as information about how to shut down.
