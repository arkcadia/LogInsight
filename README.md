# LogInsight

LogInsight is a modern C++ log analysis application designed to import heterogeneous log files, normalize their contents, detect suspicious events, and present clear analysis results.

The project aims to provide a modular and extensible foundation for log parsing, data-quality validation, rule-based detection, statistical anomaly detection, and reporting.

## Project status

LogInsight is currently under active development.

Currently implemented:

* C++20 project architecture
* Normalized log domain model
* `LogEntry` representation
* `LogLevel` enumeration
* Log-level string conversion
* Unit tests for the domain model

Currently being developed:

* Parser result and error models
* Plain-text log parser
* Parser registry
* File import service
* Command-line analysis interface

Future components:

* CSV and JSON parsing
* Configurable detection rules
* Alert generation
* SQLite persistence
* Desktop interface
* Statistical anomaly detection
* HTML and JSON reports

## Main objective

The first usable version of LogInsight will support the following workflow:

```text
Input log file
      |
      v
Format detection
      |
      v
Log parser
      |
      v
Normalized LogEntry objects
      |
      v
Data validation
      |
      v
Rule engine
      |
      v
Alerts and analysis summary
```

The initial milestone is a command-line application capable of reading a plain-text log file, normalizing valid entries, reporting malformed lines, and displaying a severity summary.

## Planned features

### Log ingestion

* Plain-text log files
* CSV files
* JSON files
* JSON Lines files
* Configurable field mappings
* Multiple files in one analysis session

### Normalization

All supported file formats will be converted into a common internal representation.

Example normalized log entry:

```cpp
struct LogEntry {
    std::uint64_t id{};
    std::chrono::system_clock::time_point timestamp{};
    LogLevel level{LogLevel::Unknown};
    std::string source;
    std::string message;
    std::string rawContent;
    std::unordered_map<std::string, std::string> metadata;
};
```

This allows the rest of the application to process logs without depending on their original format.

### Data-quality validation

LogInsight will detect issues such as:

* Invalid timestamps
* Missing fields
* Unknown severity values
* Empty messages
* Corrupted JSON objects
* Inconsistent CSV columns
* Duplicate records
* Unsupported formats

### Rule-based detection

Detection rules will be configurable and independent from the parser implementation.

Example rule:

```json
{
  "id": "database-connection-failure",
  "name": "Database connection failure",
  "enabled": true,
  "severity": "high",
  "conditions": [
    {
      "field": "message",
      "operator": "contains",
      "value": "database connection failed"
    },
    {
      "field": "level",
      "operator": "equals",
      "value": "ERROR"
    }
  ]
}
```

Planned operators include:

* `equals`
* `not_equals`
* `contains`
* `not_contains`
* `starts_with`
* `ends_with`
* `regex`
* `greater_than`
* `less_than`
* `exists`

### Event correlation

Later versions will support time-window and aggregation rules such as:

* Multiple failed logins within 60 seconds
* Repeated service restarts
* Sudden increases in error frequency
* Failure sequences followed by unavailable services
* Repeated events grouped by IP address, user, or service

### Statistical analysis

Planned anomaly-detection methods include:

* Moving averages
* Standard deviation
* Z-score analysis
* Frequency spikes
* Rare event detection
* New source detection
* Log-template extraction

The focus will remain on explainable results rather than opaque predictions.

## Technology stack

* C++20
* CMake
* GoogleTest
* `std::chrono`
* `std::filesystem`
* nlohmann/json
* spdlog
* SQLite
* Qt 6 for the future desktop interface
* GitHub Actions for continuous integration

## Project architecture

```text
loginsight/
├── CMakeLists.txt
├── README.md
├── LICENSE
├── docs/
├── include/
│   └── loginsight/
│       ├── core/
│       ├── parsers/
│       ├── rules/
│       ├── alerts/
│       └── storage/
├── src/
│   ├── core/
│   ├── parsers/
│   ├── rules/
│   ├── alerts/
│   ├── storage/
│   └── app/
├── tests/
│   ├── unit/
│   ├── integration/
│   └── fixtures/
├── resources/
│   ├── rules/
│   └── sample_logs/
└── scripts/
```

The final structure may evolve as new components are implemented.

## Design principles

LogInsight follows several architectural principles:

* Parsers understand input formats.
* The core application only understands normalized log entries.
* Parsing errors do not stop the complete import.
* Domain models remain independent from Qt.
* Business logic remains independent from the user interface.
* Storage implementation details remain behind repository interfaces.
* Each feature is added with unit or integration tests.
* Ownership and object lifetimes remain explicit.
* Global mutable state is avoided.

## Building the project

### Requirements

You will need:

* A C++20-compatible compiler
* CMake
* Git

Recommended compilers:

* GCC 13 or newer
* Clang 17 or newer

### Clone the repository

```bash
git clone https://github.com/YOUR_USERNAME/loginsight.git
cd loginsight
```

Replace `YOUR_USERNAME` with the correct GitHub username.

### Configure the project

```bash
cmake -S . -B build
```

### Build

```bash
cmake --build build
```

### Run the tests

```bash
ctest --test-dir build --output-on-failure
```

## Planned CLI usage

The command-line interface is not complete yet.

The planned usage is:

```bash
./loginsight analyse application.log
```

Expected output:

```text
Analysis complete

File: application.log
Valid entries: 196
Malformed lines: 4

Trace: 0
Debug: 12
Info: 151
Warning: 21
Error: 10
Critical: 2
Unknown: 0
```

Additional planned commands:

```bash
./loginsight analyse logs/
./loginsight validate-rules rules.json
./loginsight export --format json --output report.json
./loginsight export --format html --output report.html
```

## Supported plain-text format

The first parser will support this format:

```text
YYYY-MM-DD HH:MM:SS LEVEL SOURCE MESSAGE
```

Example:

```text
2026-07-13 18:42:10 ERROR payment-service Payment processing failed
2026-07-13 18:42:12 INFO payment-service Retrying payment request
2026-07-13 18:42:18 WARNING database Connection latency is above threshold
```

Each valid line will be converted into a normalized `LogEntry`.

Malformed lines will be reported without preventing the rest of the file from being processed.

## Testing strategy

### Unit tests

Unit tests will cover:

* Log-level conversion
* Timestamp parsing
* Individual parser behavior
* Parsing-error reporting
* Rule conditions
* Alert generation
* Summary calculation
* Storage repositories

### Integration tests

Integration tests will validate complete workflows:

```text
Fixture file
      |
      v
Parser
      |
      v
Normalization
      |
      v
Rule evaluation
      |
      v
Expected alerts
```

### Performance tests

Later benchmarks will measure:

* Parsing speed
* Memory usage
* SQLite insertion speed
* Rule-engine performance
* Processing time for large files

Planned test sizes:

* 1,000 entries
* 100,000 entries
* 1,000,000 entries

Performance results will only be published after being measured on documented hardware.

## Roadmap

### Milestone 1: Foundation

* [x] Create the C++20 project
* [x] Configure CMake
* [x] Add GoogleTest
* [x] Implement `LogEntry`
* [x] Implement `LogLevel`
* [x] Add log-level conversion tests

### Milestone 2: Plain-text ingestion

* [ ] Add `ParseError`
* [ ] Add `ParseResult`
* [ ] Add `ILogParser`
* [ ] Implement timestamp parsing
* [ ] Implement `PlainTextLogParser`
* [ ] Add realistic fixtures
* [ ] Implement `ParserRegistry`
* [ ] Implement `FileImportService`

### Milestone 3: CLI analysis

* [ ] Implement the `analyse` command
* [ ] Add severity summaries
* [ ] Add deterministic CLI formatting
* [ ] Add CLI integration tests

### Milestone 4: Additional formats

* [ ] Implement CSV parsing
* [ ] Implement JSON parsing
* [ ] Implement JSON Lines parsing
* [ ] Add configurable field mappings

### Milestone 5: Detection engine

* [ ] Define the rule format
* [ ] Implement the rule loader
* [ ] Implement condition operators
* [ ] Generate alerts
* [ ] Add aggregation rules
* [ ] Add event correlation

### Milestone 6: Persistence and reporting

* [ ] Add SQLite storage
* [ ] Add analysis sessions
* [ ] Export JSON reports
* [ ] Export HTML reports

### Milestone 7: Desktop application

* [ ] Add Qt 6
* [ ] Create the import screen
* [ ] Create the dashboard
* [ ] Create the log explorer
* [ ] Create the alert explorer
* [ ] Add background processing
* [ ] Add cancellation and progress reporting

### Milestone 8: Advanced analysis

* [ ] Add statistical anomaly detection
* [ ] Add log-template extraction
* [ ] Add rare-event detection
* [ ] Add frequency-spike detection
* [ ] Add sequence analysis

## Git workflow

Development is organized around small, focused branches.

Examples:

```text
feature/parse-result
feature/parser-interface
feature/timestamp-parser
feature/plain-text-parser
feature/parser-registry
feature/cli-analysis
```

Commit messages follow the Conventional Commits format:

```text
feat(parser): implement plain-text log parser
test(core): add log-level conversion cases
fix(parser): preserve malformed raw lines
refactor(cli): extract summary calculation
docs(readme): document project architecture
```

Before every commit:

1. Build the project.
2. Run the tests.
3. Review the diff.
4. Avoid committing generated files or secrets.
5. Stage only files related to the current task.

## Contributing

The project is currently developed as a personal software engineering project.

Suggestions, bug reports, and technical feedback are welcome through GitHub issues.

For larger changes, open an issue before creating a pull request so the proposed architecture can be discussed.

## License

The license has not yet been selected.

Before publishing or accepting external contributions, add an appropriate `LICENSE` file.

Possible options include:

* MIT License
* Apache License 2.0
* GNU General Public License v3.0

## Author

Developed by Valentin Bort-Esgueva.

LogInsight is being built as a practical project focused on modern C++, software architecture, heterogeneous data processing, testing, and explainable log analysis.
