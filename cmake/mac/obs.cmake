
if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)")
  find_package(CURL REQUIRED)

  set(OBS_EX_DEPS_DIR "${CMAKE_SOURCE_DIR}/obs-studio/.deps/obs-deps-2023-11-03-universal/include")

  target_link_libraries(SoopStudio
    PRIVATE Qt6::DBus
    CURL::libcurl
    ${LIBS_DIR}/libavcodec.dylib
    ${LIBS_DIR}/libavformat.dylib
    ${LIBS_DIR}/libavutil.dylib
    ${LIBS_DIR}/CrashReporter.framework
    "$<LINK_LIBRARY:FRAMEWORK,AppKit.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,ApplicationServices.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,AVFoundation.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,CoreServices.framework>"
   )
endif()