add_library(
  loginsight_parsers
  STATIC src/analysis_summary.cpp
         src/parser_module.cpp
         src/file_import_service.cpp
         src/import_budget.cpp
         src/json_parser_config.cpp
         src/json_lines_log_parser.cpp
         src/parser_registry.cpp
         src/plain_text_log_parser.cpp
         src/timestamp_parser.cpp
)
add_library(LogInsight::parsers ALIAS loginsight_parsers)

target_include_directories(
  loginsight_parsers
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(loginsight_parsers PUBLIC LogInsight::core PRIVATE nlohmann_json::nlohmann_json)

loginsight_enable_warnings(loginsight_parsers)
loginsight_enable_sanitizers(loginsight_parsers)
