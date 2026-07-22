if(LOGINSIGHT_ENABLE_SANITIZERS AND NOT CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
  message(
    WARNING
      "LOGINSIGHT_ENABLE_SANITIZERS is enabled, but ${CMAKE_CXX_COMPILER_ID} is not supported; "
      "sanitizer flags will not be applied."
  )
endif()

function(loginsight_enable_sanitizers target)
  if(NOT LOGINSIGHT_ENABLE_SANITIZERS)
    return()
  endif()

  if(CMAKE_CXX_COMPILER_ID MATCHES "GNU|Clang")
    target_compile_options(
      ${target}
      PRIVATE -fsanitize=address,undefined
              -fno-omit-frame-pointer
              -fno-sanitize-recover=all
    )
    target_link_options(${target} PRIVATE -fsanitize=address,undefined)
  endif()
endfunction()
