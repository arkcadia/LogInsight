# Portfolio release execution plan

Target: **v0.1.0 — Portfolio MVP**. Preserve the Qt-free domain and parser registry.
Release scope: plain-text import, severity analysis, an asynchronous Qt 6 desktop overview and
model/view explorer, bundled sample workflow, CLI help and JSON export through shared services.
Rules, alerts, JSON/CSV ingestion, statistical detection and persistence remain future work.

## Audit evidence

- No AGENTS.md, existing task specifications, or docs/CODEX_CONTEXT.md are present in this checkout.
- Existing code implements plain-text parsing, parser dispatch, summaries and CLI analysis.
- JSON configuration validation exists without a JSON parser; rules/alerts/storage expose module names only.
- README describes implemented features as unfinished and includes nonexistent paths and placeholder URLs.
- Parser silently ignores stream failures, copies parsed entries, mishandles CRLF, and timestamp conversion can overflow.
- Baseline configuration failed because system development dependencies are absent. System installation requires
  sudo credentials unavailable in this session; investigate isolated packages under /tmp for validation.
- No license file exists. Do not choose a license on the author's behalf.
- The existing generated graph is navigation evidence, not a substitute for current source review.

## Milestones

Each milestone must include implementation, affected build/tests, diff review and this log update.

| # | Objective and tasks | Acceptance criteria | Status |
|---|---|---|---|
| 1 | Audit source, tests, build, instructions and define scope | Evidence and exclusions recorded | Complete |
| 2 | Fix parser I/O, CRLF, copies and timestamp bounds; trim unused dependencies | Regression tests pass; warning-clean build | Complete |
| 3 | Shared analysis results and JSON reporting outside Qt | CLI/GUI reuse service; service/export tests | Complete |
| 4 | Optional Qt 6 GUI: dark dashboard, proxy-filtered table, details, background import | Builds; offscreen import/model/initialization tests pass | Complete |
| 5 | Synthetic multi-service demo logs and Load Sample Logs | Reproducible counts; demo works from built executable | Complete |
| 6 | CLI help, export, diagnostics and GUI state refinement | Existing CLI behavior preserved; invalid cases tested | Complete |
| 7 | Accurate README, context, architecture and release notes | Verified commands/output; genuine screenshot if captured | Complete |
| 8 | Full/headless builds, tests, sanitizers, formatting and release review | Evidence recorded; unverified criteria explicit | Complete |

## Validation log

Record changed files, executed tests, outcomes and limitations per milestone here.

### Milestone 1
- Files: docs/PORTFOLIO_RELEASE_PLAN.md.
- Checks: source/config/test inspection; git status (only existing graphify output untracked);
  baseline CMake configure (blocked by absent nlohmann_json development package).
- Review: actual capabilities determine the release scope. Alerts screen will be omitted.
- Remaining: dependency provisioning; no GUI or release behavior validated yet.

### Milestones 2–3: parser cleanup and shared services
- Files: root/core/storage/app CMake, parser/timestamp sources, new analysis_service header/source,
  parser/service unit tests and CLI integration tests, .gitignore.
- Changes: CRLF and tab handling; move normalized entries; surface stream failures; reject clock-range
  overflow; shared file validation, provenance, summary and streaming JSON export. Exclusive hard-link
  publication refuses to overwrite logs or reports. Removed unused spdlog/SQLite link requirements.
- Checks: warning-as-error Debug build; all existing tests plus regressions pass (66 headless cases).
  CLI sample produces 13 valid, 3 malformed records with expected severity counts.
- Review: no Qt dependencies in domain/parsers/application services; existing CLI summary/errors preserved.
- Limitations: complete datasets remain in memory; exports require local filesystem hard-link support.

### Milestones 4–6: desktop, demonstration and CLI polish
- Files: src/gui/*, tests/gui/*, examples/logs/*, src/app/{main.cpp,src/analysis_command.cpp},
  tests/integration/analysis_command_test.cpp, root CMake.
- Changes: optional Qt 6 Widgets/Concurrent GUI; two screens, severity dashboard, diagnostics, native
  multi-file import and drop, async immutable datasets/export snapshots, debounced message search,
  severity/source filtering, sorting and raw-record selection. No alerts UI because engine is absent.
  Each import replaces the session; completely failed batches preserve it. Busy state disables repeat
  import/export and prevents closing until work finishes. CLI adds help and --json export.
- Checks: full warning-as-error build; CTest 67/67 pass, including Qt model integrity, filters, sorting,
  asynchronous import, empty/unsupported/mixed files and failed-import preservation.
- Environment: isolated Ubuntu dependency packages under /tmp; Qt plugin path must be set for this
  environment. Normal system installs do not need those paths. Initial offscreen run failed to find
  the relocated plugin; setting QT_PLUGIN_PATH resolved it without changing application behavior.
- Review: background jobs capture values only; UI/model updates run from watcher completion on UI thread.
- Remaining: screenshots, deeper GUI/export checks, README/context and final release validation.

### Milestone 7: portfolio documentation
- Files: README.md, docs/CODEX_CONTEXT.md, docs/log-format.md, docs/RELEASE_NOTES.md,
  docs/images/{overview,explorer}.png, .github/workflows/ci.yml.
- Changes: replaced planned-feature README with actual capabilities, architecture, dependency/build/run
  commands, real sample output, supported grammar/limits, portfolio description and license status.
  Captured real application screens via QtTest after sample import and row selection; no image synthesis.
  CI matrix covers headless, desktop and sanitizer variants without unused spdlog/SQLite packages.
- Checks: viewed both screenshots, verified documented sample output/help, diff whitespace review.
- Remaining: no LICENSE exists. Author must choose a license before publishing as open source.
  Hosted CI, native dialogs and cross-platform packaging remain unverified.

### Milestone 8: release validation (in progress)
- Debug GUI build/CTest: 67/67 passed; GUI suite now contains five workflow/model tests plus setup/cleanup.
- ASan/UBSan/leak detection: 67/67 passed outside sandbox. Initial sandbox attempt failed because
  LeakSanitizer cannot operate under ptrace; no sanitizer checks/suppressions were changed.
- Expanded desktop tests: sample button, JSON export/readback, refused overwrite, busy-close behavior,
  30,000-entry import with active event loop; all passed.
- C++ formatting applied to changed files; independent Release/headless build is running.

### Milestone 8: final results

Status: **Complete, with the explicit release limitations below.**

| Configuration/check | Result |
|---|---|
| Debug desktop build + CTest (initial implementation) | 67/67 passed |
| Final Release desktop build + CTest | 68/68 passed |
| Final Release headless build + CTest | 67/67 passed |
| Final Debug desktop ASan + UBSan + leak detection | 68/68 passed, no suppressions |
| Desktop QtTest methods | 5 test methods + setup/cleanup passed |
| Executable CLI smoke tests | help=0, no command=64, unsupported=65, missing=66, export=0, overwrite=74 |
| Bundled JSON export | parsed successfully; 13 entries and 3 malformed records |
| GUI executable --sample launched from /tmp | stayed running, no startup diagnostics (offscreen) |
| Headless executable linkage | standard C++ runtime only, no Qt dependency |
| clang-format --dry-run --Werror | passed for all 18 changed C++ files |
| git diff --check | passed |
| Targeted private-key/token scan of deliverable sources/docs | no matching files |
| Screenshots | regenerated from final Release Qt application and visually inspected |

Final refinements: prevented borrowing a temporary ParserRegistry with a deleted constructor and
compile-time regression assertion; detected failed CLI output streams (including flush); tested JSON
replacement for invalid UTF-8. Status labels remain resizeable and use plain text. Formatted changed
C++ files. Reviewed changed/new source and CMake targets; no warning flags or tests were weakened.

Dependencies were downloaded/extracted under `/tmp/loginsight-deps` because sudo authentication was
unavailable. They are not vendored, installed globally, or included among release changes. These
validation commands use the same source/build options as README, plus the local dependency prefix:

```sh
export LD_LIBRARY_PATH=/tmp/loginsight-deps/usr/lib/x86_64-linux-gnu
export QT_PLUGIN_PATH=/tmp/loginsight-deps/usr/lib/x86_64-linux-gnu/qt6/plugins
cmake -S . -B build -DCMAKE_BUILD_TYPE=Release -DLOGINSIGHT_BUILD_GUI=ON \
  -DLOGINSIGHT_FETCH_GTEST=OFF -DCMAKE_PREFIX_PATH=/tmp/loginsight-deps/usr
cmake --build build --parallel 4
ctest --test-dir build --output-on-failure
```

Sanitizer build/test commands ran outside sandbox ptrace supervision, retaining leak detection.
Native GUI interaction is not claimed based on offscreen tests. No commits, pushes, releases or
Git history changes were made. Generated builds and the pre-existing graph are ignored; the graph
was marked stale because it predates these architecture changes. Screenshot PNGs are intentional docs.

## Definition of done audit

- [x] Existing domain/parser architecture and CLI remain functional; existing tests preserved.
- [x] Optional Qt 6 GUI builds and imports actual logs through application services.
- [x] Overview and model/view explorer use imported data; no fabricated alerts/statistics.
- [x] Sample loading and JSON export are reproducible and tested.
- [x] Empty, malformed, unsupported and failed input states handled explicitly.
- [x] Shared analysis services and GUI behaviors have regression tests.
- [x] README, source context, format docs, architecture and release notes updated.
- [x] Actual screenshots/sample output included; build/test/run commands verified with local dependency prefix.
- [x] Formatting, compiler warnings, headless build and sanitizers validated.
- [x] Rules/alerts screen omitted because no engine exists (conditional criterion not applicable).
- [ ] Native file dialogs, visual accessibility and desktop interaction: **unverified** manually.
- [ ] Windows/macOS and standalone binary packaging: **unverified**, not claimed.
- [ ] Hosted GitHub Actions matrix execution: **unverified**, configuration prepared only.
- [ ] License selection/open-source publication readiness: **pending author decision**.

Remaining product limits: full dataset/raw records in RAM, filtering/sorting on UI thread, no
cancellation, hard-link-capable filesystem and unused path required for report publication. JSON/CSV
input, rule evaluation, persistence and statistical analysis remain roadmap items. The working MVP
is prepared; a public open-source release still needs the author's license choice.

## Additional portfolio screenshots

Captured `docs/images/showcase-explorer.png` and `showcase-source-filter.png` from the actual
Release GUI using the bundled sample, at 1.5x pixel density. The full timeline selects the critical
worker event; the second capture filters to the worker service and shows its failure/recovery context.
Input path is relative (`examples/logs/atlas.log`), so screenshots do not expose a workstation path.
No image generation, overlays, fabricated records, or UI restyling was used. README links both views.

Capture review exposed a stale explorer count after proxy-filter invalidation. Connected layout-change
notifications to the count/detail refresh and added regression assertions for `5 of 13 entries` and
retained critical-event details after the worker source filter. Rebuilt affected GUI targets; desktop_workflow passed with the regression.
