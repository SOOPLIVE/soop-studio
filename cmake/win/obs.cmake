cmake_minimum_required(VERSION 3.19)

if (WIN32)

file(READ ${CMAKE_SOURCE_DIR}/obs-studio/buildspec.json BUILD_SPEC_CONTENTS)
string(JSON DEPS_VERSION GET ${BUILD_SPEC_CONTENTS} "dependencies" "prebuilt" "version")

set(OBS_EX_DEPS_DIR "${CMAKE_SOURCE_DIR}/obs-studio/.deps/obs-deps-${DEPS_VERSION}-x64/include")
set(OBS_WIN_THR_DEPS_DIR_ "${CMAKE_SOURCE_DIR}/obs-studio/deps/w32-pthreads")


target_include_directories(SoopStudio PRIVATE ${OBS_WIN_THR_DEPS_DIR_})

set(obs_fronted_api_out_dir ${CMAKE_SOURCE_DIR}/obs-studio/build_x64/UI/obs-frontend-api/$<CONFIG>)
set(obs_dep_lib_dir "${CMAKE_SOURCE_DIR}/obs-studio/.deps/obs-deps-${DEPS_VERSION}-x64/lib")
string(REPLACE "/" "\\" __obs_fronted_api_out_dir ${obs_fronted_api_out_dir})
string(REPLACE "/" "\\" __obs_dep_lib_dir ${obs_dep_lib_dir})
string(REPLACE "/" "\\" __CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR})

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__obs_fronted_api_out_dir}\\*.lib" "${__CMAKE_SOURCE_DIR}\\lib" /s /f /y /i
                   COMMENT "Running Copy libs 1")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__obs_dep_lib_dir}\\libcurl_imp.lib" "${__CMAKE_SOURCE_DIR}\\lib" /s /f /y /i
                   COMMENT "Running Copy libs 2")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__obs_dep_lib_dir}\\avcodec.lib" "${__CMAKE_SOURCE_DIR}\\lib" /s /f /y /i
                   COMMENT "Running Copy libs 3")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__obs_dep_lib_dir}\\avformat.lib" "${__CMAKE_SOURCE_DIR}\\lib" /s /f /y /i
                   COMMENT "Running Copy libs 4")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__obs_dep_lib_dir}\\avutil.lib" "${__CMAKE_SOURCE_DIR}\\lib" /s /f /y /i
                   COMMENT "Running Copy libs 5")

add_custom_command(TARGET SoopStudio
                   PRE_BUILD
                   COMMAND xcopy "${__obs_dep_lib_dir}\\detours.lib" "${__CMAKE_SOURCE_DIR}\\lib" /s /f /y /i
                   COMMENT "Running Copy libs 6")


target_link_libraries(SoopStudio
	PRIVATE 
  ${LIBS_DIR}/libcurl_imp.lib
  ${LIBS_DIR}/avcodec.lib
  ${LIBS_DIR}/avformat.lib
  ${LIBS_DIR}/avutil.lib
  ${LIBS_DIR}/detours.lib
  ${obs_dep_lib_dir}/zlib.lib
  ${obs_dep_lib_dir}/zlibstatic.lib
)


find_package(Qt6Svg REQUIRED)
target_link_libraries(SoopStudio
	PRIVATE Qt6::Svg
)

find_package(Qt6SvgWidgets REQUIRED)
target_link_libraries(SoopStudio
	PRIVATE Qt6::SvgWidgets
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd\\\"4828\\\"")
endif()