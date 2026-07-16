if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)")
	add_executable(SoopStudio
    ${ALL_SRC_LIST}
    ${ALL_INCLUDE}
    ${ALL_FORM_LIST}
    ${ALL_RES_LIST}
    ${ALL_ASSET_LIST}
    ${ALL_SRC_LIST_PLATFORM}
    ${ALL_MM_FILE_LIST}
	)
endif()