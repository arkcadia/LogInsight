set(LOGINSIGHT_TEST_FIXTURES_DIR tests/fixtures)

add_executable(
  loginsight_integration_tests
  analysis_command_test.cpp
  module_smoke_test.cpp
)

add_dependencies(loginsight_integration_tests loginsight_copy_test_fixtures)

target_compile_definitions(
  loginsight_integration_tests
  PRIVATE LOGINSIGHT_TEST_FIXTURES_DIR="${LOGINSIGHT_TEST_FIXTURES_DIR}"
)

target_link_libraries(
  loginsight_integration_tests
  PRIVATE LogInsight::app_cli
          LogInsight::alerts
          LogInsight::parsers
          LogInsight::rules
          LogInsight::storage
          GTest::gtest_main
)

loginsight_enable_warnings(loginsight_integration_tests)

include(GoogleTest)
gtest_discover_tests(loginsight_integration_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
