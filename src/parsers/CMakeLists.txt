add_library(loginsight_parsers STATIC src/parser_module.cpp)
add_library(LogInsight::parsers ALIAS loginsight_parsers)

target_include_directories(
  loginsight_parsers
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(loginsight_parsers PUBLIC LogInsight::core)

loginsight_enable_warnings(loginsight_parsers)
