add_executable(loginsight_gui_tests gui_test.cpp)
target_link_libraries(loginsight_gui_tests PRIVATE loginsight_gui_components Qt6::Test)
set_target_properties(loginsight_gui_tests PROPERTIES AUTOMOC ON)
target_compile_definitions(loginsight_gui_tests PRIVATE
  LOGINSIGHT_SAMPLE_PATH="${PROJECT_SOURCE_DIR}/examples/logs/atlas.log")
loginsight_enable_warnings(loginsight_gui_tests)
loginsight_enable_sanitizers(loginsight_gui_tests)
add_test(NAME desktop_workflow COMMAND loginsight_gui_tests)
set_tests_properties(desktop_workflow PROPERTIES ENVIRONMENT "QT_QPA_PLATFORM=offscreen" TIMEOUT 60)

add_custom_target(loginsight_copy_gui_test_samples
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:loginsight_gui_tests>/samples"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/examples/logs/atlas.log" "$<TARGET_FILE_DIR:loginsight_gui_tests>/samples/atlas.log")
add_dependencies(loginsight_gui_tests loginsight_copy_gui_test_samples)
