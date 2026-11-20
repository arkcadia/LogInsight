add_library(loginsight_gui_components STATIC comparison_panel.cpp large_log_panel.cpp live_panel.cpp log_model.cpp main_window.cpp malformed_panel.cpp recap_panel.cpp)
target_include_directories(loginsight_gui_components PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
target_link_libraries(loginsight_gui_components PUBLIC LogInsight::application LogInsight::rules Qt6::Widgets Qt6::Concurrent PRIVATE SQLite::SQLite3)
set(sample_dir "${CMAKE_INSTALL_DATADIR}/loginsight/samples")
file(RELATIVE_PATH installed_sample "${CMAKE_INSTALL_FULL_BINDIR}"
  "${CMAKE_INSTALL_FULL_DATADIR}/loginsight/samples/atlas.log")
target_compile_definitions(loginsight_gui_components PRIVATE LOGINSIGHT_INSTALLED_SAMPLE="${installed_sample}")
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

install(TARGETS loginsight_gui RUNTIME DESTINATION ${CMAKE_INSTALL_BINDIR})
install(FILES "${PROJECT_SOURCE_DIR}/examples/logs/atlas.log" DESTINATION "${sample_dir}")
if(CMAKE_SYSTEM_NAME STREQUAL "Linux")
  install(FILES "${PROJECT_SOURCE_DIR}/packaging/loginsight.desktop"
    DESTINATION "${CMAKE_INSTALL_DATADIR}/applications")
endif()
