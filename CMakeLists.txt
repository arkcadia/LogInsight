cmake_minimum_required(VERSION 3.24)

project(
  LogInsight
  VERSION 0.1.0
  DESCRIPTION "Modern C++20 log analysis application"
  LANGUAGES CXX
)

option(LOGINSIGHT_BUILD_TESTS "Build LogInsight tests" ON)
option(LOGINSIGHT_FETCH_GTEST "Download GoogleTest if it is not found locally" ON)
option(
  LOGINSIGHT_ENABLE_SANITIZERS
  "Enable AddressSanitizer and UndefinedBehaviorSanitizer for LogInsight targets"
  OFF
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED ON)
set(CMAKE_CXX_EXTENSIONS OFF)

set(CMAKE_EXPORT_COMPILE_COMMANDS ON)

include(cmake/CompilerWarnings.cmake)
include(cmake/Sanitizers.cmake)

find_package(nlohmann_json CONFIG REQUIRED)
find_package(spdlog CONFIG REQUIRED)
find_package(SQLite3 REQUIRED)

add_subdirectory(src/core)
add_subdirectory(src/parsers)
add_subdirectory(src/rules)
add_subdirectory(src/alerts)
add_subdirectory(src/storage)
add_subdirectory(src/app)

if(LOGINSIGHT_BUILD_TESTS)
  enable_testing()

  find_package(GTest CONFIG QUIET)
  if(NOT GTest_FOUND)
    find_package(GTest QUIET)
  endif()
  if(NOT GTest_FOUND)
    if(NOT LOGINSIGHT_FETCH_GTEST)
      message(
        FATAL_ERROR
          "GoogleTest was not found. Install it, set GTest_DIR/CMAKE_PREFIX_PATH, "
          "or configure with -DLOGINSIGHT_FETCH_GTEST=ON."
      )
    endif()

    include(FetchContent)
    FetchContent_Declare(
      googletest
      GIT_REPOSITORY https://github.com/google/googletest.git
      GIT_TAG v1.15.2
    )
    set(gtest_force_shared_crt
        ON
        CACHE BOOL "" FORCE
    )
    FetchContent_MakeAvailable(googletest)
  endif()

  add_subdirectory(tests/unit)
  add_subdirectory(tests/integration)
endif()
