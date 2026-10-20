add_executable(
  loginsight_unit_tests
  analysis_summary_formatter_test.cpp
  analysis_summary_test.cpp
  analysis_service_test.cpp
  core_version_test.cpp
  file_import_service_test.cpp
  json_parser_config_test.cpp
  log_entry_test.cpp
  log_level_test.cpp
  log_parser_interface_test.cpp
  parser_registry_test.cpp
  parse_result_test.cpp
  plain_text_log_parser_test.cpp
  timestamp_parser_test.cpp
)

set(LOGINSIGHT_TEST_FIXTURES_DIR tests/fixtures)

add_custom_target(
  loginsight_copy_test_fixtures
  COMMAND ${CMAKE_COMMAND} -E copy_directory ${CMAKE_SOURCE_DIR}/tests/fixtures
          ${CMAKE_BINARY_DIR}/${LOGINSIGHT_TEST_FIXTURES_DIR}
)

add_dependencies(loginsight_unit_tests loginsight_copy_test_fixtures)

target_compile_definitions(
  loginsight_unit_tests
  PRIVATE LOGINSIGHT_TEST_FIXTURES_DIR="${LOGINSIGHT_TEST_FIXTURES_DIR}"
)

target_link_libraries(
  loginsight_unit_tests
  PRIVATE LogInsight::app_cli LogInsight::core LogInsight::parsers GTest::gtest_main nlohmann_json::nlohmann_json
)

loginsight_enable_warnings(loginsight_unit_tests)
loginsight_enable_sanitizers(loginsight_unit_tests)

include(GoogleTest)
gtest_discover_tests(loginsight_unit_tests WORKING_DIRECTORY ${CMAKE_BINARY_DIR})
