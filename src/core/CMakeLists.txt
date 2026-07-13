add_library(loginsight_core STATIC src/log_level.cpp src/version.cpp)
add_library(LogInsight::core ALIAS loginsight_core)

target_include_directories(
  loginsight_core
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(loginsight_core PUBLIC nlohmann_json::nlohmann_json spdlog::spdlog)

loginsight_enable_warnings(loginsight_core)
