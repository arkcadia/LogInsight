add_executable(loginsight_integration_tests module_smoke_test.cpp)

target_link_libraries(
  loginsight_integration_tests
  PRIVATE LogInsight::alerts
          LogInsight::parsers
          LogInsight::rules
          LogInsight::storage
          GTest::gtest_main
)

loginsight_enable_warnings(loginsight_integration_tests)

include(GoogleTest)
gtest_discover_tests(loginsight_integration_tests)
