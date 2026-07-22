add_library(
  loginsight_app_cli
  STATIC src/analysis_command.cpp
         src/analysis_summary_formatter.cpp
)
add_library(LogInsight::app_cli ALIAS loginsight_app_cli)

target_include_directories(
  loginsight_app_cli
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(loginsight_app_cli PUBLIC LogInsight::parsers)

loginsight_enable_warnings(loginsight_app_cli)

add_executable(loginsight_app main.cpp)

target_link_libraries(
  loginsight_app
  PRIVATE LogInsight::app_cli
)

set_target_properties(loginsight_app PROPERTIES OUTPUT_NAME loginsight)

loginsight_enable_warnings(loginsight_app)
