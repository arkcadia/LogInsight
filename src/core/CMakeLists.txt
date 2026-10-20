add_library(loginsight_core STATIC src/log_level.cpp src/version.cpp)
add_library(LogInsight::core ALIAS loginsight_core)

target_include_directories(
  loginsight_core
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

target_compile_features(loginsight_core PUBLIC cxx_std_20)

loginsight_enable_warnings(loginsight_core)
loginsight_enable_sanitizers(loginsight_core)
