# v0.1.0 — Portfolio MVP

Suggested release notes; no release, binary publication or Git commit has been created.

LogInsight now provides a working local log-analysis workflow in a Qt 6 desktop application
and a command-line tool, backed by the same C++20 parser and application services.

## Included

- Dark overview with real severity distribution, file counts and import diagnostics.
- Model/view log explorer with message search, severity/source filters, sorting and raw details.
- Background multi-file import/export, drag-and-drop, explicit busy/error/empty states.
- One-click synthetic sample session with 13 valid events and 3 malformed records.
- CLI help, preserved summary/exit-code behavior and complete JSON session export.
- Parser fixes for CRLF, tabs, stream failures and out-of-range timestamps.
- Optional GUI build, reduced dependencies and separate application/CLI/desktop targets.
- Service, parser, CLI and Qt regression coverage; genuine GUI screenshots and verified sample output.

## Architecture

Plain-text files → parser registry/import service → normalized LogEntry/ParseError values →
application analysis/report service → CLI or asynchronous Qt presentation. Qt never enters
the domain/parser/application dependencies. Dataset ownership is immutable once shared with UI.

## Validation

See PORTFOLIO_RELEASE_PLAN.md for exact configurations, results and environmental notes.
Local validation uses Ubuntu 26.04, GCC 15.2 and Qt 6.10.2. Automated desktop checks run
offscreen; this does not establish native Windows/macOS or desktop accessibility support.

- Final Release desktop: 68/68 CTest cases passed.
- Final Release headless: 67/67 passed, without Qt linkage.
- Final Debug ASan/UBSan/leak detection desktop: 68/68 passed.
- CLI sample/export/exit-code checks and formatting/diff checks passed.

## Known limits and publication decision

- **Author action required:** select a license. No LICENSE file exists; this is not yet an
  explicitly licensed open-source release. No license has been invented by this task.
- Plain-text `.log`/`.txt` only. No JSON/CSV ingestion, rule alerts, persistence or statistical analysis.
- Sessions/raw records remain in memory. Filtering/sorting runs on the UI thread; import/export
  runs in workers. There is no cancellation and closing waits for processing to finish.
- Each import replaces the current session. Export includes all files/entries, not only table matches.
- JSON publication requires hard-link support and an unused output path.
- Native dialogs, accessibility, installers and other OS platforms need manual validation.
- GitHub CI configuration has been updated but its hosted execution was not observed locally.

## Suggested Conventional Commit messages

```text
fix(parsers): handle CRLF and read failures and bound timestamp conversion
feat(app): share analysis services and add safe JSON report export
feat(gui): add asynchronous Qt overview and log explorer with sample workflow
test(release): cover desktop workflows and headless sanitizer configurations
docs(portfolio): add genuine screenshots and verified release guidance
```
