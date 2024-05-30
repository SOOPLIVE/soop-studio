cmake_minimum_required(VERSION 3.22...3.25)


project(SoopStudio LANGUAGES CXX C)


#======================= INCLUSION OF Qt =======================#
set(CMAKE_INCLUDE_CURRENT_DIR ON)
set(CMAKE_AUTOMOC ON)
set(CMAKE_AUTOUIC ON)
set(CMAKE_AUTORCC ON)
set(CMAKE_PREFIX_PATH $ENV{QTDIR})

find_package(Qt6Core REQUIRED)
find_package(Qt6Widgets REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS UiTools)
find_package(Qt6 COMPONENTS Network REQUIRED)
find_package(Qt6Svg REQUIRED)
find_package(Qt6Network REQUIRED)

if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)")
  find_package(Qt6 REQUIRED COMPONENTS DBus)
endif()


#=================== INCLUSION OF Project Files ====================#
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(INCLUDE_DIR "${CMAKE_SOURCE_DIR}/include")
set(FORMS_DIR "${CMAKE_SOURCE_DIR}/src/ui-forms")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
set(LIBS_DIR "${CMAKE_SOURCE_DIR}/lib")
set(CSS_DIR "${CMAKE_SOURCE_DIR}/src/ui-forms/qss")

#
if (WIN32)
  set(PLATFORM_DIR "${CMAKE_SOURCE_DIR}/platform/win")
endif()
if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)")
  set(PLATFORM_DIR "${CMAKE_SOURCE_DIR}/platform/mac")
endif()
#
set(LIBOBS_DIR "${CMAKE_SOURCE_DIR}/obs-studio-clone/libobs")
set(OBS_UI_DIR "${CMAKE_SOURCE_DIR}/obs-studio-clone/UI")
set(OBS_FEUI_DIR "${CMAKE_SOURCE_DIR}/obs-studio-clone/UI/obs-frontend-api")
set(OBS_DEPS_DIR "${CMAKE_SOURCE_DIR}/obs-studio-clone/deps")
set(OBS_JSON_DEPS_DIR "${CMAKE_SOURCE_DIR}/obs-studio-clone/deps/json11")


file(GLOB_RECURSE ALL_SRC_LIST 
     "${SOURCE_DIR}/*.cpp*"
     "${SOURCE_DIR}/*.h*"
     "${SOURCE_DIR}/*.hpp*"
     "${SOURCE_DIR}/*.inl*")

file(GLOB_RECURSE ALL_INCLUDE 
     "${INCLUDE_DIR}/*.h*"
     "${INCLUDE_DIR}/*.hpp*"
     "${INCLUDE_DIR}/*.inl*")

file(GLOB_RECURSE ALL_FORM_LIST 
     "${FORMS_DIR}/*.ui*")

file(GLOB_RECURSE ALL_RES_LIST 
     "${FORMS_DIR}/*.jpeg*"
     "${FORMS_DIR}/*.jpg*"
     "${FORMS_DIR}/*.png*"
     "${FORMS_DIR}/*.svg*"
     "${FORMS_DIR}/*.qrc*")

file(GLOB_RECURSE ALL_ASSET_LIST 
     "${ASSETS_DIR}/*.jpeg*"
     "${ASSETS_DIR}/*.jpg*"
     "${ASSETS_DIR}/*.png*"
     "${ASSETS_DIR}/*.svg*")

file(GLOB_RECURSE ALL_SRC_LIST_PLATFORM
    "${PLATFORM_DIR}/*.mm*"
    "${PLATFORM_DIR}/*.c*"
    "${PLATFORM_DIR}/*.cpp*"
    "${PLATFORM_DIR}/*.h*"
    "${PLATFORM_DIR}/*.hpp*"
    "${PLATFORM_DIR}/*.inl*")



foreach(_source IN ITEMS ${ALL_SRC_LIST})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

foreach(_form IN ITEMS ${ALL_FORM_LIST})
    get_filename_component(_form_path "${_form}" PATH)
    source_group("UI" FILES "${_form}")
endforeach()

foreach(_res IN ITEMS ${ALL_RES_LIST})
    get_filename_component(_res_path "${_res}" PATH)
    string(REPLACE "${FORMS_DIR}" "" _group_path "${_res_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("UI\\res\\${_group_path}" FILES "${_res}")
endforeach()

foreach(_asset IN ITEMS ${ALL_ASSET_LIST})
    get_filename_component(_asset_path "${_asset}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_asset_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_asset}")
endforeach()

foreach(_source_platform IN ITEMS ${ALL_SRC_LIST_PLATFORM})
    get_filename_component(_source_platform_path "${_source_platform}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_platform_path}")
    if (WIN32) 
       string(REPLACE "platform/win" "src" _group_path "${_group_path}")
    endif()
    if (CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)") 
       string(REPLACE "platform/mac" "src" _group_path "${_group_path}")
    endif()
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source_platform}")
endforeach()


include_directories(
	${FORMS_DIR}
	${INCLUDE_DIR}
	${SOURCE_DIR}
#	${CSS_DIR}
  ${PLATFORM_DIR}
  ${LIBOBS_DIR}
  ${OBS_DEPS_DIR}
)


#=================== SETUP EXECTUABLE ====================#
# Enable debug logging on RELWITHDEBINFO configuration
set_property(DIRECTORY APPEND PROPERTY COMPILE_DEFINITIONS
    $<$<CONFIG:RELWITHDEBINFO>:QT_MESSAGELOGCONTEXT>
)

# Add the forms directory to the AUTOUIC search paths
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${FORMS_DIR})


# Add the executable
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

if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)")
	add_executable(SoopStudio
    ${ALL_SRC_LIST}
    ${ALL_INCLUDE}
    ${ALL_FORM_LIST}
    ${ALL_RES_LIST}
    ${ALL_ASSET_LIST}
    ${ALL_SRC_LIST_PLATFORM}
	)
endif()


# Add the target includes for SoopStudio
target_include_directories(SoopStudio PRIVATE ${FORMS_DIR})
target_include_directories(SoopStudio PRIVATE ${INCLUDE_DIR})
target_include_directories(SoopStudio PRIVATE ${SOURCE_DIR})
target_include_directories(SoopStudio PRIVATE ${PLATFORM_DIR})

target_include_directories(SoopStudio PRIVATE ${OBS_DEPS_DIR})
target_include_directories(SoopStudio PRIVATE ${OBS_UI_DIR})
target_include_directories(SoopStudio PRIVATE ${OBS_FEUI_DIR})
target_include_directories(SoopStudio PRIVATE ${OBS_JSON_DEPS_DIR})

add_subdirectory("${CMAKE_SOURCE_DIR}/obs-studio-clone/deps/json11" "${CMAKE_BINARY_DIR}/deps/json11")



#---------- obs-browser setting  ----------#
add_library(browser-panels INTERFACE)
add_library(OBS::browser-panels ALIAS browser-panels)

#target_sources(browser-panels INTERFACE panel/browser-panel.hpp)

target_include_directories(browser-panels INTERFACE "${CMAKE_SOURCE_DIR}/obs-studio-clone/plugins/obs-browser/panel")

target_compile_definitions(browser-panels INTERFACE BROWSER_AVAILABLE)

target_link_libraries(SoopStudio PRIVATE OBS::browser-panels)



#----------------------- OBS DEPS -----------------------#
if (WIN32)
set(OBS_EX_DEPS_DIR "${CMAKE_SOURCE_DIR}/obs-studio-clone/.deps/obs-deps-2023-11-03-x64/include")
set(OBS_WIN_THR_DEPS_DIR_ "${CMAKE_SOURCE_DIR}/obs-studio-clone/deps/w32-pthreads")


target_include_directories(SoopStudio PRIVATE ${OBS_WIN_THR_DEPS_DIR_})

set(obs_fronted_api_out_dir ${CMAKE_SOURCE_DIR}/obs-studio-clone/build_x64/UI/obs-frontend-api/$<CONFIG>)
set(obs_dep_lib_dir ${CMAKE_SOURCE_DIR}/obs-studio-clone/.deps/obs-deps-2023-11-03-x64/lib)
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
)


find_package(Qt6Svg REQUIRED)
target_link_libraries(SoopStudio
	PRIVATE Qt6::Svg
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} /wd\\\"4828\\\"")
endif()

if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)")
  find_package(CURL REQUIRED)

  set(OBS_EX_DEPS_DIR "${CMAKE_SOURCE_DIR}/obs-studio-clone/.deps/obs-deps-2023-11-03-universal/include")

  target_link_libraries(SoopStudio
    PRIVATE Qt6::DBus
    CURL::libcurl
    ${LIBS_DIR}/libavcodec.dylib
    ${LIBS_DIR}/libavformat.dylib
    ${LIBS_DIR}/libavutil.dylib
    "$<LINK_LIBRARY:FRAMEWORK,AppKit.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,ApplicationServices.framework>"
    "$<LINK_LIBRARY:FRAMEWORK,AVFoundation.framework>"
   )
endif()


target_include_directories(SoopStudio PRIVATE ${OBS_EX_DEPS_DIR})





#===================== LINKING LIBRARIES =======================#

target_link_libraries(SoopStudio
	PRIVATE Qt6::UiTools
  OBS::json11
  Qt6::Svg
  Qt6::Network
  $<IF:$<PLATFORM_ID:Windows>,
   ${LIBS_DIR}/obs.lib, 
   ${LIBS_DIR}/libobs.framework/Versions/A/libobs>
  $<IF:$<PLATFORM_ID:Windows>,
   ${LIBS_DIR}/obs-frontend-api.lib, 
   ${LIBS_DIR}/obs-frontend-api.dylib>

)


#===================== IDE PROJECT SETTINGS =======================#
#-------------------------       VC       -------------------------#
if (WIN32)
  # For Fast Build
  target_compile_options(SoopStudio PRIVATE /MP)


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

set(obs_lib_out_dir ${CMAKE_SOURCE_DIR}/obs-studio-clone/build_x64/libobs/$<CONFIG>)
set(obs_run_bin_out_dir ${CMAKE_SOURCE_DIR}/obs-studio-clone/build_x64/rundir/$<CONFIG>)
string(REPLACE "/" "\\" __obs_lib_out_dir ${obs_lib_out_dir})
string(REPLACE "/" "\\" __obs_run_bin_out_dir ${obs_run_bin_out_dir})
string(REPLACE "/" "\\" __CMAKE_SOURCE_DIR ${CMAKE_SOURCE_DIR})
string(REPLACE "/" "\\" __CMAKE_BINARY_DIR ${CMAKE_BINARY_DIR})
string(REPLACE "/" "\\" __CSS_DIR ${CSS_DIR})

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
                   COMMAND xcopy "${__obs_run_bin_out_dir}\\obs-plugins" "${__CMAKE_BINARY_DIR}\\$<CONFIG>\\obs-plugins" /e /f /k /y /exclude:${__CMAKE_SOURCE_DIR}\\obs-plugin-xcopy-exclude-list.txt /i
                   COMMENT "Running Copy OBS bin files")
endif()



#-------------------------     XCODE     -------------------------#
# target_install_resources: Helper function to add resources into bundle
function(target_install_resources target)
  message(DEBUG "Installing resources for target ${target}...")
  if(EXISTS "${OBS_UI_DIR}/data")
    file(GLOB_RECURSE data_files "${OBS_UI_DIR}/data/*")
    list(FILTER data_files EXCLUDE REGEX "locale")
    list(FILTER data_files EXCLUDE REGEX "themes")
    foreach(data_file IN LISTS data_files)
      cmake_path(RELATIVE_PATH data_file BASE_DIRECTORY "${OBS_UI_DIR}/data/" OUTPUT_VARIABLE
                 relative_path)
      cmake_path(GET relative_path PARENT_PATH relative_path)
      target_sources(${target} PRIVATE "${data_file}")
      set_property(SOURCE "${data_file}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${relative_path}")
      source_group("Resources/${relative_path}" FILES "${data_file}")
    endforeach()
  endif()
endfunction()

function(target_install_custom_themes_res target)
  message(DEBUG "Installing Custom Themes for target ${target}...")
  if(EXISTS "${CSS_DIR}")
    file(GLOB_RECURSE data_files "${CSS_DIR}/*")
    foreach(data_file IN LISTS data_files)
      target_sources(${target} PRIVATE "${data_file}")
      set_property(SOURCE "${data_file}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources/themes")
      source_group("Resources/themes" FILES "${data_file}")
    endforeach()
  endif()
endfunction()

function(target_install_custom_assets_res target)
  message(DEBUG "Installing Custom Assets for target ${target}...")
  if(EXISTS "${ASSETS_DIR}")
    file(GLOB_RECURSE data_files "${ASSETS_DIR}/*")
    foreach(data_file IN LISTS data_files)
      cmake_path(RELATIVE_PATH data_file BASE_DIRECTORY "${ASSETS_DIR}/" OUTPUT_VARIABLE
                 relative_path)
      cmake_path(GET relative_path PARENT_PATH relative_path)
      target_sources(${target} PRIVATE "${data_file}")
      set_property(SOURCE "${data_file}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources/assets/${relative_path}")
      source_group("Resources/assets" FILES "${data_file}")
    endforeach()
  endif()
endfunction()

# set_target_xcode_properties: Sets Xcode-specific target attributes
function(set_target_xcode_properties target)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 0 _STXP "${options}" "${oneValueArgs}" "${multiValueArgs}")

  message(DEBUG "Setting Xcode properties for target ${target}...")

  while(_STXP_PROPERTIES)
    list(POP_FRONT _STXP_PROPERTIES key value)
    # cmake-format: off
    set_property(TARGET ${target} PROPERTY XCODE_ATTRIBUTE_${key} "${value}")
    # cmake-format: on
  endwhile()
endfunction()


if(CMAKE_HOST_SYSTEM_NAME MATCHES "(Darwin)")
  set_target_properties(
    SoopStudio
    PROPERTIES OUTPUT_NAME SOOPStudio
                MACOSX_BUNDLE TRUE
                XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "13.0"
                #MACOSX_BUNDLE_INFO_PLIST "${CMAKE_CURRENT_SOURCE_DIR}/cmake/macos/Info.plist.in"
                XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY YES
                XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES
                XCODE_EMBED_PLUGINS_REMOVE_HEADERS_ON_COPY YES
                XCODE_EMBED_PLUGINS_CODE_SIGN_ON_COPY YES
                XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME[variant=Release] YES
                XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME[variant=RelWithDebInfo] YES
                XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME[variant=MinSizeRel] YES
                )

  set_target_xcode_properties(
    SoopStudio
    PROPERTIES PRODUCT_BUNDLE_IDENTIFIER com.soop.studio
               PRODUCT_NAME SOOPStudio
               ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
               LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"
               #CURRENT_PROJECT_VERSION ${_BUILD_NUMBER}
               #MARKETING_VERSION ${_VERSION_CANONICAL}
               GENERATE_INFOPLIST_FILE YES
               COPY_PHASE_STRIP NO
               CLANG_ENABLE_OBJC_ARC YES
               SKIP_INSTALL NO
               INSTALL_PATH "$(LOCAL_APPS_DIR)"
               INFOPLIST_KEY_CFBundleDisplayName "SOOPStudio"
               INFOPLIST_KEY_NSHumanReadableCopyright "(c) 2024-${CURRENT_YEAR} SOOP LIVE Corp"
               INFOPLIST_KEY_NSCameraUsageDescription "SOOPStudio needs to access the camera to enable camera sources to work."
               INFOPLIST_KEY_NSMicrophoneUsageDescription "SOOPStudio needs to access the microphone to enable audio input.")


  set(_QT_INSTALL_DIR /opt/homebrew/Cellar/qt/6.5.2/lib) # will change path

  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libobs.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/obs-frontend-api.dylib)

  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtCore.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtGui.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtUiTools.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtWidgets.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtNetwork.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtOpenGLWidgets.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtOpenGL.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtSvg.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtDBus.framework)

  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libobs-opengl.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libavcodec.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libavutil.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libavformat.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libswscale.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libswresample.dylib)

  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/librist.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libsrt.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libx264.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libmbedtls.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libmbedcrypto.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libmbedx509.dylib)

  get_property(_frameworks GLOBAL PROPERTY _FRAMEWORKS)

  set_property(
    TARGET SoopStudio
    APPEND
    PROPERTY XCODE_EMBED_FRAMEWORKS ${_frameworks})


  target_install_resources(SoopStudio)
  target_install_custom_themes_res(SoopStudio)
  target_install_custom_assets_res(SoopStudio)

  set(LOCALE_INI_TARGET ${CMAKE_SOURCE_DIR}/localeIni/locale.ini)
  target_sources(SoopStudio PRIVATE ${LOCALE_INI_TARGET})
  set_source_files_properties(${LOCALE_INI_TARGET} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  set(APP_ICON_TARGET ${ASSETS_DIR}/macIcon/Assets.xcassets)
  target_sources(SoopStudio PRIVATE ${APP_ICON_TARGET})
  set_source_files_properties(${APP_ICON_TARGET} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  #target_add_resource(SoopStudio "${ASSETS_DIR}/macIcon/Assets.xcassets")

  # Find and add Qt plugin libraries associated with Qt component to target
  set(plugins_list)
  
  file(GLOB plugin_libraries 
     "$ENV{QTDIR}/plugins/iconengines/*.dylib*"
     "$ENV{QTDIR}/plugins/imageformats/*.dylib*"
     "$ENV{QTDIR}/plugins/platforms/*.dylib*"
     "$ENV{QTDIR}/plugins/styles/*.dylib*")

  foreach(plugin_library IN ITEMS ${plugin_libraries})
    list(APPEND plugins_list ${plugin_library})
  endforeach()

  list(REMOVE_DUPLICATES plugins_list)

  foreach(plugin IN LISTS plugins_list)
    message(TRACE ${plugin})
    cmake_path(GET plugin PARENT_PATH plugin_path)
    set(plugin_base_dir "${plugin_path}/../")
    cmake_path(SET plugin_stem_dir NORMALIZE "${plugin_base_dir}")
    cmake_path(RELATIVE_PATH plugin_path BASE_DIRECTORY "${plugin_stem_dir}" OUTPUT_VARIABLE plugin_file_name)
    target_sources(SoopStudio PRIVATE "${plugin}")
    set_source_files_properties("${plugin}" PROPERTIES MACOSX_PACKAGE_LOCATION "plugins/${plugin_file_name}"
                                                       XCODE_FILE_ATTRIBUTES "CodeSignOnCopy")
    source_group("Qt plugins" FILES "${plugin}")
  endforeach()
  #
  

  set(obs_lib_out_dir_mac ${CMAKE_SOURCE_DIR}/obs-studio-clone/build_macos/libobs/$<CONFIG>)
  set(obs_run_bin_out_dir_mac ${CMAKE_SOURCE_DIR}/obs-studio-clone/build_macos/UI/$<CONFIG>/OBS.app/Contents/Frameworks)

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND cp -Rf "${obs_lib_out_dir_mac}/*" "${LIBS_DIR}"
                     COMMENT "Running Copy OBS core libs")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND for libfile in ${obs_run_bin_out_dir_mac}/*.dylib\; do cp -f $libfile ${LIBS_DIR}\; done
                     COMMENT "Running Copy OBS deps")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND rm -Rf "${LIBS_DIR}/QtCore.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtGui.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtUiTools.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtWidgets.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtOpenGLWidgets.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtOpenGL.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtSvg.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtDBus.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtNetwork.framework"
                     COMMENT "Running Remove Qt bin")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtCore.framework" "${LIBS_DIR}/QtCore.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtGui.framework" "${LIBS_DIR}/QtGui.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtUiTools.framework" "${LIBS_DIR}/QtUiTools.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtWidgets.framework" "${LIBS_DIR}/QtWidgets.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtOpenGLWidgets.framework" "${LIBS_DIR}/QtOpenGLWidgets.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtOpenGL.framework" "${LIBS_DIR}/QtOpenGL.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtSvg.framework" "${LIBS_DIR}/QtSvg.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtDBus.framework" "${LIBS_DIR}/QtDBus.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtNetwork.framework" "${LIBS_DIR}/QtNetwork.framework"
                     COMMENT "Running Copy Qt bin")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND for libfile in ${LIBS_DIR}/*.dylib\; do codesign -s \"Developer ID Application: AfreecaTVCoLtd \(K6NJ2QFV2P\)\" -f $libfile\; done
                     COMMENT "Running Force codesign OBS deps")

  
endif()
