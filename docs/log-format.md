# LogInsight Normalized Log Format

This document records the current domain-model decisions for normalized log data. It is not a
parser specification yet.

## Scope

The normalized model lives in `loginsight::core` and is independent from Qt. Parser modules can
later translate specific input formats into these value types, but the value types do not know
which parser produced them.

## LogEntry

`LogEntry` is the canonical in-memory representation of one log event.

- `timestamp` uses `std::chrono::system_clock::time_point`.
- `level` uses the `LogLevel` enum.
- `source` uses `LogSource`.
- `message` stores the normalized human-readable log message.
- `attributes` stores optional string key/value fields discovered by a parser.

The model uses plain structs with value semantics. There is no inheritance hierarchy because the
current domain data does not need polymorphic behavior.

## LogLevel

`LogLevel` contains:

- `trace`
- `debug`
- `info`
- `warning`
- `error`
- `critical`
- `unknown`

Canonical string conversion returns lowercase names. Parsing is case-insensitive and accepts a few
common aliases:

- `information` maps to `info`
- `warn` maps to `warning`
- `err` maps to `error`
- `fatal` maps to `critical`

Unrecognized strings return `std::nullopt`. The explicit `unknown` level is kept for entries whose
severity is absent or intentionally unknown after normalization.

## LogSource

`LogSource` identifies where an entry came from without imposing a parser-specific shape.

- `name` is the display/source name, such as a file name, stream name, or service name.
- `host` is optional because local files and imported logs may not carry host data.
- `application` is optional because not every source identifies a process or service.
- `file_path` is optional because some sources are streams, databases, or APIs rather than files.

Paths are stored as strings for now so the domain model can represent paths from systems that do
not match the local platform.

## Attributes

`LogAttributes` is currently `std::map<std::string, std::string>` to keep ordering deterministic in
tests, serialization, and UI presentation. Values remain strings until a later feature needs typed
attributes.

## Current plain-text input and reports

The plain-text parser is implemented for `.log`/`.txt`. Records use
`YYYY-MM-DD HH:MM:SS LEVEL SOURCE MESSAGE` (space or `T` inside the timestamp;
spaces/tabs between fields). Zone-free timestamps are interpreted as UTC. LF/CRLF and
blank lines are supported. Canonical severities plus WARN are accepted; the core's wider
alias conversion is not the plain-text grammar. No `Z` suffix, fractional seconds or timezone
offsets are accepted in plain-text records. The standalone timestamp helper also supports
`Z`, but the record parser intentionally uses a fixed 19-character timestamp.

`rawContent` preserves the input record excluding its LF/CRLF line terminator. Import services
attach the input path to `source.file_path`. Malformed records retain line number/raw content/reason;
line zero denotes an I/O failure rather than a malformed record. Application services exclude
I/O-failed files from valid session counts. JSON schema version 1 exports UTC timestamps with
`Z`, entries, summary counts and all malformed records; this is an output format, not an input parser.
