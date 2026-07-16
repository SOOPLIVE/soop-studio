if (WIN32) 
  add_executable(SoopStudio 
	  WIN32 ${ALL_SRC_LIST}
    ${ALL_INCLUDE}
    ${ALL_FORM_LIST}
    ${ALL_RES_LIST}
    ${ALL_ASSET_LIST}
    ${ALL_SRC_LIST_PLATFORM}
  )
endif()