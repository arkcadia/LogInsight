# LogInsight

LogInsight is a C++20 desktop application for parsing, normalizing, and inspecting log data. This
first release focuses on the foundation: a normalized log domain model, a reusable parser interface,
a plain-text parser, timestamp parsing utilities, fixture-backed tests, and a minimal Qt 6 desktop
shell.

The codebase is organized around small modules under [`src/`](src/). Parser behavior is covered by
readable fixtures under [`tests/fixtures/`](tests/fixtures/), and the current plain-text parser
supports this format:

```text
YYYY-MM-DD HH:MM:SS LEVEL SOURCE MESSAGE
```

Example:

```text
2026-07-13 18:42:10 ERROR payment-service Payment processing failed
```

## Notable Techniques

- **Target-based CMake**  
  The build uses target-scoped linking and include paths instead of global include directories. This
  keeps module dependencies visible and local. See CMake's
  [`target_link_libraries`](https://cmake.org/cmake/help/latest/command/target_link_libraries.html)
  documentation.

- **Value-based domain models**  
  `LogEntry`, `LogSource`, `ParseError`, and `ParseResult` use plain public structures. This keeps
  ownership clear and avoids unnecessary inheritance in the domain layer.

- **Parser interface with explicit result reporting**  
  [`ILogParser`](src/parsers/include/loginsight/parsers/log_parser.hpp) returns `ParseResult`, so
  parsers can return valid entries and malformed lines from the same import without throwing
  exceptions for ordinary bad input.

- **C++20 timestamp validation**  
  [`timestamp_parser.cpp`](src/parsers/src/timestamp_parser.cpp) uses C++20 chrono calendar types to
  reject invalid dates instead of normalizing them. See `std::chrono::year_month_day` on
  [cppreference](https://en.cppreference.com/cpp/chrono/year_month_day).

- **Fixture-backed parser tests**  
  Parser tests copy [`tests/fixtures/`](tests/fixtures/) into the build tree, then load files by
  stable relative paths. This avoids hardcoded absolute paths and keeps test data easy to inspect.

- **Qt-free parser and domain layers**  
  The core and parser modules do not depend on Qt. Qt is isolated to the application shell under
  [`src/app/`](src/app/).

- **No browser runtime**  
  This release does not use browser APIs such as the
  [Intersection Observer API](https://developer.mozilla.org/en-US/docs/Web/API/Intersection_Observer_API),
  CSS scrolling APIs, or web fonts. The UI target is native Qt Widgets, not a DOM-based frontend.

## Technologies And Libraries

- [CMake](https://cmake.org/) for the build system.
- [Qt 6 Widgets](https://doc.qt.io/qt-6/qtwidgets-index.html) for the desktop GUI shell.
- [GoogleTest](https://google.github.io/googletest/) for unit and integration tests.
- [nlohmann/json](https://json.nlohmann.me/) is configured for future structured log and settings
  work.
- [spdlog](https://github.com/gabime/spdlog) is configured for application logging.
- [SQLite](https://sqlite.org/docs.html) is configured for the storage layer.
- C++20 standard library features, especially `std::chrono`, `std::optional`,
  `std::filesystem`, and value-oriented containers.

No specific fonts are used in this release.

## Project Structure

```text
.
├── .clang-format
├── .gitignore
├── CMakeLists.txt
├── README.md
├── cmake/
├── docs/
├── src/
│   ├── alerts/
│   │   ├── include/
│   │   └── src/
│   ├── app/
│   ├── core/
│   │   ├── include/
│   │   └── src/
│   ├── parsers/
│   │   ├── include/
│   │   └── src/
│   ├── rules/
│   │   ├── include/
│   │   └── src/
│   └── storage/
│       ├── include/
│       └── src/
└── tests/
    ├── fixtures/
    ├── integration/
    └── unit/
```

[`src/core/`](src/core/) contains the normalized domain model and shared application metadata.
[`src/parsers/`](src/parsers/) contains parser contracts, parse result models, timestamp parsing,
and the plain-text parser. [`src/app/`](src/app/) contains the minimal Qt 6 executable.
[`docs/`](docs/) records format and domain decisions. [`tests/fixtures/`](tests/fixtures/) contains
small log samples used by parser tests. [`tests/unit/`](tests/unit/) and
[`tests/integration/`](tests/integration/) separate fast unit coverage from broader module smoke
tests.

## Current Release Scope

This first release establishes the project foundation:

- Normalized log models: `LogEntry`, `LogLevel`, and `LogSource`
- Parser result models: `ParseError` and `ParseResult`
- Shared parser interface: `ILogParser`
- Plain-text parser for timestamp, level, source, and message fields
- Timestamp parsing utility for space-separated, ISO-style, and UTC-suffixed timestamps
- Fixture-backed GoogleTest coverage
- Minimal Qt 6 GUI executable
- Modular CMake targets with strict compiler warnings

Storage, alerting, rule evaluation, and richer GUI workflows are present as module boundaries but
are not implemented yet.
