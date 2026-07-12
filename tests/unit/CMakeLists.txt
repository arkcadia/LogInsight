add_executable(loginsight_unit_tests core_version_test.cpp)

target_link_libraries(loginsight_unit_tests PRIVATE LogInsight::core GTest::gtest_main)

loginsight_enable_warnings(loginsight_unit_tests)

include(GoogleTest)
gtest_discover_tests(loginsight_unit_tests)
