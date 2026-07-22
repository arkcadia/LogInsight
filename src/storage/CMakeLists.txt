add_library(loginsight_storage STATIC src/storage_module.cpp)
add_library(LogInsight::storage ALIAS loginsight_storage)

target_include_directories(
  loginsight_storage
  PUBLIC $<BUILD_INTERFACE:${CMAKE_CURRENT_SOURCE_DIR}/include>
         $<INSTALL_INTERFACE:include>
)

target_link_libraries(loginsight_storage PUBLIC LogInsight::core SQLite::SQLite3)

loginsight_enable_warnings(loginsight_storage)
loginsight_enable_sanitizers(loginsight_storage)
