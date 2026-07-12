add_executable(loginsight_app main.cpp)

target_link_libraries(
  loginsight_app
  PRIVATE LogInsight::alerts
          LogInsight::core
          LogInsight::parsers
          LogInsight::rules
          LogInsight::storage
          Qt6::Widgets
)

set_target_properties(
  loginsight_app
  PROPERTIES OUTPUT_NAME LogInsight
             WIN32_EXECUTABLE TRUE
             MACOSX_BUNDLE TRUE
)

loginsight_enable_warnings(loginsight_app)
