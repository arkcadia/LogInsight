add_library(loginsight_alerts STATIC src/alerts_module.cpp)
add_library(LogInsight::alerts ALIAS loginsight_alerts)

target_include_directories(
  loginsight_alerts
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(loginsight_alerts PUBLIC LogInsight::core LogInsight::rules)

loginsight_enable_warnings(loginsight_alerts)
loginsight_enable_sanitizers(loginsight_alerts)
