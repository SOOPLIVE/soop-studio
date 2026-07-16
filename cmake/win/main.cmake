if (WIN32)
  # For Fast Build
  target_compile_options(SoopStudio PRIVATE /MP)


target_compile_definitions(SoopStudio PRIVATE
                            NOMINMAX                          # Use the standard's templated min/max
                            _CRT_SECURE_NO_WARNINGS
                            _CRT_NONSTDC_NO_WARNINGS
                            PSAPI_VERSION=2
                            )
                                

set_target_properties(
  SoopStudio
  PROPERTIES WIN32_EXECUTABLE TRUE
             RUNTIME_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/bin/64bit"
             VS_DEBUGGER_COMMAND "${CMAKE_BINARY_DIR}/$<CONFIG>/bin/64bit/SoopStudio.exe"
             VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/bin/64bit")
 
set_property(DIRECTORY ${CMAKE_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT SoopStudio)



configure_file(${FORMS_DIR}/soopstudio.rc.in ${CMAKE_BINARY_DIR}/soopstudio.rc)

target_sources(
	SoopStudio 
	PRIVATE 
	${CMAKE_BINARY_DIR}/soopstudio.rc)

target_link_options(SoopStudio PRIVATE "$<$<CONFIG:Release>:/MAP>")

set(obs_lib_out_dir ${CMAKE_SOURCE_DIR}/obs-studio/build_x64/libobs/$<CONFIG>)
set(obs_run_bin_out_dir ${CMAKE_SOURCE_DIR}/obs-studio/build_x64/rundir/$<CONFIG>)
string(REPLACE "/" "\\" __obs_lib_out_dir ${obs_lib_out_dir})
string(REPLACE "/" "\\" __obs_run_bin_out_dir ${obs_run_bin_out_dir})
string(REPLACE "/" "\\" __CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR})
string(REPLACE "/" "\\" __CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR})
string(REPLACE "/" "\\" __CSS_DIR ${CSS_DIR})
string(REPLACE "/" "\\" __LOCALE_DATA_DIR ${LOCALE_DATA_DIR})

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__obs_lib_out_dir}\\*.lib" "${__CMAKE_SOURCE_DIR}\\lib" /s /f /y /i
                   COMMENT "Running Copy OBS core libs")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__obs_run_bin_out_dir}\\bin" "${__CMAKE_BINARY_DIR}\\$<CONFIG>\\bin" /e /f /k /y /exclude:${__CMAKE_SOURCE_DIR}\\obs-bin-xcopy-exclude-list.txt
                   COMMENT "Running Copy OBS bin files")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__obs_run_bin_out_dir}\\data" "${__CMAKE_BINARY_DIR}\\$<CONFIG>\\data" /e /f /k /y /exclude:${__CMAKE_SOURCE_DIR}\\obs-data-xcopy-exclude-list.txt /i
                   COMMENT "Running Copy OBS bin files")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__CSS_DIR}" "${__CMAKE_BINARY_DIR}\\$<CONFIG>\\data\\obs-studio\\themes" /e /f /k /y /i
                   COMMENT "Running Copy OBS bin files")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__LOCALE_DATA_DIR}" "${__CMAKE_BINARY_DIR}\\$<CONFIG>\\data\\obs-studio\\locale" /e /f /k /y /i
                   COMMENT "Running Copy OBS bin files")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__obs_run_bin_out_dir}\\obs-plugins" "${__CMAKE_BINARY_DIR}\\$<CONFIG>\\obs-plugins" /e /f /k /y /exclude:${__CMAKE_SOURCE_DIR}\\obs-plugin-xcopy-exclude-list.txt /i
                   COMMENT "Running Copy OBS bin files")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND robocopy "${CMAKE_SOURCE_DIR}/assets" "${CMAKE_BINARY_DIR}/$<CONFIG>/data/obs-studio/assets" /mir /mt:8 /r:3 /w:5 || ver > nul
                   COMMENT "Running Miror assets files")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND robocopy "${CMAKE_SOURCE_DIR}/plugins/vlc" "${CMAKE_BINARY_DIR}/$<CONFIG>/obs-plugins/64bit/vlc" /mir /mt:8 /r:3 /w:5 || ver > nul
                   COMMENT "Running Miror vlc files")                   

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__CMAKE_SOURCE_DIR}\\localeIni\\locale.ini" "${__CMAKE_BINARY_DIR}\\$<CONFIG>\\data\\obs-studio" /d /f /k /y
                   COMMENT "Running Copy locale.ini")

              
endif()
