cmake_minimum_required(VERSION 3.22...3.25)

option(ENABLE_PLUGINS "Enable building SOOP plugins" ON)

# APPLE/WIN32/UNIX are soft-deprecated: https://discourse.cmake.org/t/platform-id-vs-win32-vs-cmake-system-name/1226/2
if(WIN32)
  add_subdirectory(soop-spout2)
  add_subdirectory(soop-streamdeck)
endif()
  add_subdirectory(soop-filters)