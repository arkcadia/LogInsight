# LogInsight maintenance context

## Implemented release scope

C++20 plain-text log analysis with a Qt-free domain/parser/application stack. Qt 6 desktop
and CLI share analysis and JSON reporting. Desktop: overview, real severity distribution,
import diagnostics, searchable/filterable/sortable QTableView, raw details, native/drop import,
async workers, JSON export and bundled sample session. Every import replaces the session;
fully failed imports keep the old session. No alerts UI.

## Modules and ownership

- core: LogEntry/LogSource/LogLevel, chrono timestamp and value semantics; no external libraries.
- parsers: ILogParser, unique_ptr-owned ParserRegistry, borrowing FileImportService, ParseResult,
  PlainTextLogParser, timestamp validation and severity summary.
- app: LogInsight::application owns the default parser registry through AnalysisService; file
  validation, summary/provenance and streamed JSON. LogInsight::app_cli formats CLI output.
- gui: worker creates Dataset, then shared_ptr<const Dataset> hands immutable files/entry
  pointers/source choices to UI. Dataset is noncopyable because entry pointers refer into files.
  All QObjects/model resets occur on UI thread. Workers capture values, never the main window.
- rules/alerts/storage: module-name scaffolding only. JSON parser config is validated but unused
  for ingestion. Preserve existing tests without advertising these modules as implemented features.

## Input/report contracts

Plain text: YYYY-MM-DD[space or T]HH:MM:SS LEVEL SOURCE MESSAGE, spaces/tabs, LF/CRLF,
case-insensitive canonical levels plus WARN; `.log`/`.txt` extension matching. No timezone
suffix/fractional seconds. Timestamps interpreted as UTC and checked against system_clock bounds.
Malformed records do not abort import. Line zero denotes a file/stream I/O failure.

JSON schema_version 1: files array, each containing path, completed, diagnostic, summary,
entries and malformed_records. Invalid UTF-8 is replaced on export. Export is the entire session,
not filtered table rows. It stages a temporary sibling then creates an exclusive hard link;
existing destinations are never overwritten. Hard-link support is required.

CLI: analyse <file> [--json <new-path>], --help. Exit codes 0/64/65/66/74/78 documented in README.

## Builds and checks

Use README commands. GUI is opt-in (LOGINSIGHT_BUILD_GUI=ON); headless requires no Qt.
System dependencies: nlohmann-json3-dev, libgtest-dev; GUI additionally qt6-base-dev.
Strict warnings include -Werror. Sanitizers are opt-in with LOGINSIGHT_ENABLE_SANITIZERS.
Existing GoogleTest cases are preserved. QtTest exercises model/workflow offscreen.
Screenshots in docs/images are actual application captures from the GUI test.

For this agent environment only, dependencies were unpacked under /tmp/loginsight-deps.
Configure with -DCMAKE_PREFIX_PATH=/tmp/loginsight-deps/usr and use
LD_LIBRARY_PATH=/tmp/loginsight-deps/usr/lib/x86_64-linux-gnu and
QT_PLUGIN_PATH=/tmp/loginsight-deps/usr/lib/x86_64-linux-gnu/qt6/plugins.
These temporary paths are not project requirements or portable deployment instructions.

## Remaining limits

Full dataset/raw text in RAM, UI-thread filtering/sorting, no cancellation; closing waits for
work to finish. Diagnostics show at most 20 reasons per file, full errors are exported.
No rules, alerts, statistics, JSON/CSV input, persistence, installers or cross-platform claims.
No license selected; author decision still needed. No publish/commit authorization exists.
See PORTFOLIO_RELEASE_PLAN.md and RELEASE_NOTES.md for validation and release evidence.

The existing graphify output predates portfolio changes and is marked stale; rebuild it before relying on it for new architecture queries.
