add_library(loginsight_gui_components STATIC log_model.cpp main_window.cpp)
target_include_directories(loginsight_gui_components PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(loginsight_gui_components PUBLIC LogInsight::application Qt6::Widgets Qt6::Concurrent)
loginsight_enable_warnings(loginsight_gui_components)
loginsight_enable_sanitizers(loginsight_gui_components)

add_executable(loginsight_gui main.cpp)
set_target_properties(loginsight_gui PROPERTIES OUTPUT_NAME loginsight-gui)
target_link_libraries(loginsight_gui PRIVATE loginsight_gui_components)
loginsight_enable_warnings(loginsight_gui)
loginsight_enable_sanitizers(loginsight_gui)
# Keep the demonstration next to the executable, independent of source checkout/current directory.
add_custom_target(loginsight_copy_samples ALL
  COMMAND ${CMAKE_COMMAND} -E make_directory "$<TARGET_FILE_DIR:loginsight_gui>/samples"
  COMMAND ${CMAKE_COMMAND} -E copy_if_different
    "${PROJECT_SOURCE_DIR}/examples/logs/atlas.log" "$<TARGET_FILE_DIR:loginsight_gui>/samples/atlas.log")
add_dependencies(loginsight_gui loginsight_copy_samples)
