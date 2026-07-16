if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)")
  find_package(Qt6 REQUIRED COMPONENTS DBus)
endif()