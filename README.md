# LogInsight

LogInsight is a modern C++20 desktop application skeleton for log analysis.

This repository currently contains the initial architecture only. Business logic is intentionally
not implemented yet.

## Requirements

- CMake 3.24 or newer
- A C++20 compiler
- Qt 6 Widgets
- GoogleTest
- nlohmann/json
- spdlog
- SQLite development libraries

The CMake project uses package config discovery where available:

- `find_package(Qt6 REQUIRED COMPONENTS Widgets)`
- `find_package(nlohmann_json CONFIG REQUIRED)`
- `find_package(spdlog CONFIG REQUIRED)`
- `find_package(SQLite3 REQUIRED)`

GoogleTest is used for tests. CMake first searches for an installed package and then falls back
to downloading GoogleTest with `FetchContent` when `LOGINSIGHT_FETCH_GTEST` is enabled.

## Build

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug
cmake --build build
```

If dependencies are installed in a custom prefix, pass it through `CMAKE_PREFIX_PATH`:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/prefix
cmake --build build
```

## Test

```sh
ctest --test-dir build --output-on-failure
```

Tests can be disabled with:

```sh
cmake -S . -B build -DLOGINSIGHT_BUILD_TESTS=OFF
```

If you built GoogleTest from source and installed it to a custom prefix, point CMake at that
prefix:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DCMAKE_PREFIX_PATH=/path/to/gtest/install
```

If GoogleTest was built but not installed, either install it first or let this project fetch it:

```sh
cmake -S . -B build -DCMAKE_BUILD_TYPE=Debug -DLOGINSIGHT_FETCH_GTEST=ON
```

## Project Layout

```text
src/
  core/       Shared core utilities and application metadata
  parsers/    Log parser module boundary
  rules/      Rule engine module boundary
  alerts/     Alerting module boundary
  storage/    Persistence module boundary backed by SQLite
  app/        Qt 6 GUI executable
tests/
  unit/        Unit tests
  integration/ Integration tests
```

## CMake Notes

- Uses C++20 with `CMAKE_CXX_EXTENSIONS` disabled.
- Uses target-based CMake.
- Uses target-scoped include directories only.
- Enables strict compiler warnings through `loginsight_enable_warnings`.
