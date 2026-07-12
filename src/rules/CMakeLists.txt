add_library(loginsight_rules STATIC src/rules_module.cpp)
add_library(LogInsight::rules ALIAS loginsight_rules)

target_include_directories(
  loginsight_rules
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(loginsight_rules PUBLIC LogInsight::core)

loginsight_enable_warnings(loginsight_rules)
