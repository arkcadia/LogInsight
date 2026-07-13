add_executable(
  loginsight_unit_tests
  core_version_test.cpp
  log_entry_test.cpp
  log_level_test.cpp
  log_parser_interface_test.cpp
  parse_result_test.cpp
  plain_text_log_parser_test.cpp
)

target_compile_definitions(
  loginsight_unit_tests
  PRIVATE LOGINSIGHT_TEST_FIXTURES_DIR="${CMAKE_SOURCE_DIR}/tests/fixtures"
)

target_link_libraries(
  loginsight_unit_tests
  PRIVATE LogInsight::core LogInsight::parsers GTest::gtest_main
)

loginsight_enable_warnings(loginsight_unit_tests)

include(GoogleTest)
gtest_discover_tests(loginsight_unit_tests)
