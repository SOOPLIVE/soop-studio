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

function(target_install_custom_locale_res target)
  message(DEBUG "Installing Custom Locales for target ${target}...")
  if(EXISTS "${LOCALE_DATA_DIR}")
    file(GLOB_RECURSE data_files "${LOCALE_DATA_DIR}/*")
    foreach(data_file IN LISTS data_files)
      target_sources(${target} PRIVATE "${data_file}")
      set_property(SOURCE "${data_file}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources/locale")
      source_group("Resources/locale" FILES "${data_file}")
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
  set(CMAKE_XCODE_ATTRIBUTE_ENABLE_PARALLEL_BUILD "YES")
  set(CMAKE_XCODE_ATTRIBUTE_ENABLE_FAST_BUILD YES)
  # precompile header 고민 해보자 - davin
  #set_target_properties(SoopStudio PROPERTIES
  #  XCODE_ATTRIBUTE_GCC_PRECOMPILE_PREFIX_HEADER YES
  #  XCODE_ATTRIBUTE_GCC_PREFIX_HEADER "${CMAKE_SOURCE_DIR}/pch.h"
  #)

  set_target_properties(
    SoopStudio
    PROPERTIES OUTPUT_NAME SOOPStudio
                MACOSX_BUNDLE TRUE
                XCODE_ATTRIBUTE_MACOSX_DEPLOYMENT_TARGET "13.0"
		MACOSX_BUNDLE_INFO_PLIST ${MACOSX_BUNDLE_INFO_PLIST}                
                XCODE_EMBED_FRAMEWORKS_REMOVE_HEADERS_ON_COPY YES
                XCODE_EMBED_FRAMEWORKS_CODE_SIGN_ON_COPY YES
                XCODE_EMBED_PLUGINS_REMOVE_HEADERS_ON_COPY YES
                XCODE_EMBED_PLUGINS_CODE_SIGN_ON_COPY YES
                XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME[variant=Release] YES
                XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME[variant=RelWithDebInfo] YES
                XCODE_ATTRIBUTE_ENABLE_HARDENED_RUNTIME[variant=MinSizeRel] YES
                XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Debug] dwarf
                XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=RelWithDebInfo] dwarf
                XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=Release] dwarf-with-dsym
                XCODE_ATTRIBUTE_DEBUG_INFORMATION_FORMAT[variant=MinSizeRel] dwarf-with-dsym
                # 코드 공개 시 조심.. - Davin
                XCODE_ATTRIBUTE_PROVISIONING_PROFILE_SPECIFIER "GP_SOOP_Studio_Developer_ID"
                # 코드 공개 시 조심.. - Davin
                XCODE_ATTRIBUTE_CODE_SIGN_ENTITLEMENTS "${CMAKE_SOURCE_DIR}/SoopStudio.entitlements"
                )

  set_target_xcode_properties(
    SoopStudio
    PROPERTIES PRODUCT_BUNDLE_IDENTIFIER com.soop.studio
               PRODUCT_NAME SOOPStudio
               ASSETCATALOG_COMPILER_APPICON_NAME AppIcon
               LD_RUNPATH_SEARCH_PATHS "@executable_path/../Frameworks"
               #CURRENT_PROJECT_VERSION ${_BUILD_NUMBER}
               CURRENT_PROJECT_VERSION 1.1.2
               #MARKETING_VERSION ${_VERSION_CANONICAL}
               MARKETING_VERSION 1.1.2
               GENERATE_INFOPLIST_FILE YES
               COPY_PHASE_STRIP NO
               CLANG_ENABLE_OBJC_ARC YES
               SKIP_INSTALL NO
               INSTALL_PATH "$(LOCAL_APPS_DIR)"
               # 코드 공개 시 조심.. - Davin
               DEVELOPMENT_TEAM "K6NJ2QFV2P"
               # 코드 공개 시 조심.. - Davin
               CODE_SIGN_IDENTITY "Developer ID Application"
               INFOPLIST_KEY_CFBundleDisplayName "SOOPStudio"
               INFOPLIST_KEY_NSHumanReadableCopyright "(c) 2024-${CURRENT_YEAR} SOOP Corp"
               INFOPLIST_KEY_NSCameraUsageDescription "SOOPStudio needs to access the camera to enable camera sources to work."
               INFOPLIST_KEY_NSMicrophoneUsageDescription "SOOPStudio needs to access the microphone to enable audio input.")


  target_compile_definitions(SoopStudio PRIVATE $<$<CONFIG:Debug>:_DEBUG>)

  set(_QT_INSTALL_DIR /opt/homebrew/Cellar/qt/6.5.2/lib) # will change path
  set(_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR ${CMAKE_SOURCE_DIR}/mac-sub-frameworks)

  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libobs.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/obs-frontend-api.dylib)

  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS "${LIBS_DIR}/Chromium Embedded Framework.framework")
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/Syphon.framework)

  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtCore.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtGui.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtUiTools.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtWidgets.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtNetwork.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtOpenGLWidgets.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtOpenGL.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtSvg.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtSvgWidgets.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtDBus.framework)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/QtXml.framework)

  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libobs-opengl.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libavcodec.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libavdevice.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libavfilter.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libavutil.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libavformat.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libswscale.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libswresample.dylib)

  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libdatachannel.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libfreetype.dylib)

  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/librist.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libsrt.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libx264.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libmbedtls.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libmbedcrypto.dylib)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${LIBS_DIR}/libmbedx509.dylib)


  set_property(GLOBAL APPEND PROPERTY _BROWSER_HELPER "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/CrashReporter.framework")
  set_property(GLOBAL APPEND PROPERTY _BROWSER_HELPER "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper (GPU).app")
  set_property(GLOBAL APPEND PROPERTY _BROWSER_HELPER "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper (Plugin).app")
  set_property(GLOBAL APPEND PROPERTY _BROWSER_HELPER "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper (Renderer).app")
  set_property(GLOBAL APPEND PROPERTY _BROWSER_HELPER "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper.app")


  get_property(_browser_helper GLOBAL PROPERTY _BROWSER_HELPER)
  set_property(GLOBAL APPEND PROPERTY _FRAMEWORKS ${_browser_helper})

  foreach(_PATH ${_browser_helper})
    if(NOT EXISTS "${_PATH}")
      file(MAKE_DIRECTORY "${_PATH}")  #browser_helper 빈 폴더라도 미리 생성
    endif()
  endforeach()
  



  get_property(_frameworks GLOBAL PROPERTY _FRAMEWORKS)

  set_property(
    TARGET SoopStudio
    APPEND
    PROPERTY XCODE_EMBED_FRAMEWORKS ${_frameworks})


  target_install_resources(SoopStudio)
  target_install_custom_themes_res(SoopStudio)
  target_install_custom_locale_res(SoopStudio)
  target_install_custom_assets_res(SoopStudio)

  set(LOCALE_INI_TARGET ${CMAKE_SOURCE_DIR}/localeIni/locale.ini)
  target_sources(SoopStudio PRIVATE ${LOCALE_INI_TARGET})
  set_source_files_properties(${LOCALE_INI_TARGET} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)

  set(APP_ICON_TARGET ${ASSETS_DIR}/macIcon/Assets.xcassets)
  target_sources(SoopStudio PRIVATE ${APP_ICON_TARGET})
  set_source_files_properties(${APP_ICON_TARGET} PROPERTIES MACOSX_PACKAGE_LOCATION Resources)
  #target_add_resource(SoopStudio "${ASSETS_DIR}/macIcon/Assets.xcassets")

  set(MAC_CONTENTS ${CMAKE_SOURCE_DIR}/mac-contents/obs-ffmpeg-mux)
  target_sources(SoopStudio PRIVATE ${MAC_CONTENTS})
  set_source_files_properties(${MAC_CONTENTS} PROPERTIES MACOSX_PACKAGE_LOCATION MacOS)

  file(GLOB_RECURSE license_files "${CMAKE_SOURCE_DIR}/license/*")
  foreach(license_file IN LISTS license_files)
    target_sources(SoopStudio PRIVATE "${license_file}")
    set_source_files_properties("${license_file}" PROPERTIES MACOSX_PACKAGE_LOCATION "Resources/license")
    source_group("Resources/license" FILES "${license_file}")
  endforeach()

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


  # obs plugin 빈 폴더라도 미리 생성
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/aja-output-ui.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/aja.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/coreaudio-encoder.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/decklink-captions.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/decklink-output-ui.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/decklink.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/frontend-tools.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/image-source.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/mac-avcapture.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/mac-capture.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/mac-syphon.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/mac-videotoolbox.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/mac-virtualcam.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/obs-browser.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/obs-ffmpeg.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/obs-filters.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/obs-outputs.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/obs-transitions.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/obs-vst.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/obs-webrtc.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/obs-websocket.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/obs-x264.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/rtmp-services.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/text-freetype2.plugin)
  set_property(GLOBAL APPEND PROPERTY _PLUGINS ${CMAKE_SOURCE_DIR}/pluginData/mac/vlc-video.plugin)

  get_property(_plugins GLOBAL PROPERTY _PLUGINS)

  foreach(_PATH ${_plugins})
    if(NOT EXISTS "${_PATH}")
      file(MAKE_DIRECTORY "${_PATH}")
    endif()
  endforeach()
  #

  # copy custom obs plugin
  file(GLOB custom_obs_plugins "${CMAKE_SOURCE_DIR}/pluginData/mac/*")
  foreach(custom_obs_plugin IN LISTS custom_obs_plugins)
    target_sources(SoopStudio PRIVATE "${custom_obs_plugin}")
    set_source_files_properties("${custom_obs_plugin}" PROPERTIES MACOSX_PACKAGE_LOCATION "plugins"
                                                                  XCODE_FILE_ATTRIBUTES "CodeSignOnCopy")
    source_group("OBS Plugins" FILES "${custom_obs_plugin}")
  endforeach()
  #
  

  set(obs_lib_out_dir_mac ${CMAKE_SOURCE_DIR}/obs-studio/build_macos/libobs/$<CONFIG>)
  set(obs_run_out_dir_mac ${CMAKE_SOURCE_DIR}/obs-studio/build_macos/UI/$<CONFIG>/OBS.app/Contents)
  set(obs_run_bin_out_dir_mac ${obs_run_out_dir_mac}/Frameworks)
  set(obs_run_plugins_out_dir_mac ${obs_run_out_dir_mac}/PlugIns)

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND rm -Rf "${LIBS_DIR}"
                     COMMAND mkdir "${LIBS_DIR}"
                     COMMAND cp -Rf "${obs_lib_out_dir_mac}/*" "${LIBS_DIR}"
                     COMMENT "Running Copy OBS core libs")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND for libfile in ${obs_run_bin_out_dir_mac}/*.dylib\; do cp -f $libfile ${LIBS_DIR}\; done
                     COMMENT "Running Copy OBS deps")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND rm -Rf "${LIBS_DIR}/Chromium Embedded Framework.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/Syphon.framework"
                     COMMENT "Running Remove OBS deps framework")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND cp -Rf "${obs_run_bin_out_dir_mac}/Chromium Embedded Framework.framework" "${LIBS_DIR}/Chromium Embedded Framework.framework"
                     COMMAND cp -Rf "${obs_run_bin_out_dir_mac}/Syphon.framework" "${LIBS_DIR}/Syphon.framework"
                     COMMENT "Running Copy OBS deps framework")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND rm -Rf "${CMAKE_SOURCE_DIR}/pluginData/mac/*"
                     COMMENT "Running Remove OBS plugins")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/aja-output-ui.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/aja-output-ui.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/aja.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/aja.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/coreaudio-encoder.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/coreaudio-encoder.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/decklink-captions.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/decklink-captions.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/decklink-output-ui.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/decklink-output-ui.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/decklink.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/decklink.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/frontend-tools.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/frontend-tools.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/image-source.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/image-source.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/mac-avcapture.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/mac-avcapture.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/mac-capture.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/mac-capture.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/mac-syphon.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/mac-syphon.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/mac-videotoolbox.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/mac-videotoolbox.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/mac-virtualcam.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/mac-virtualcam.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/obs-browser.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/obs-browser.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/obs-ffmpeg.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/obs-ffmpeg.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/obs-filters.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/obs-filters.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/obs-outputs.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/obs-outputs.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/obs-transitions.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/obs-transitions.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/obs-vst.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/obs-vst.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/obs-webrtc.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/obs-webrtc.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/obs-websocket.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/obs-websocket.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/obs-x264.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/obs-x264.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/rtmp-services.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/rtmp-services.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/text-freetype2.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/text-freetype2.plugin"
                     COMMAND cp -Rf "${obs_run_plugins_out_dir_mac}/vlc-video.plugin" "${CMAKE_SOURCE_DIR}/pluginData/mac/vlc-video.plugin"
                     COMMENT "Running Copy OBS plugins")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND rm -Rf "${LIBS_DIR}/CrashReporter.framework"
                     COMMAND rm -Rf "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper \\(GPU\\).app"
                     COMMAND rm -Rf "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper \\(Plugin\\).app"
                     COMMAND rm -Rf "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper \\(Renderer\\).app"
                     COMMAND rm -Rf "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper.app"
                     COMMENT "Running Remove frameworks")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND cp -Rf "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/CrashReporter.framework" "${LIBS_DIR}/CrashReporter.framework"
                     COMMAND cp -Rf "${obs_run_bin_out_dir_mac}/SOOPStudio Helper \\(GPU\\).app" "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper \\(GPU\\).app"
                     COMMAND cp -Rf "${obs_run_bin_out_dir_mac}/SOOPStudio Helper \\(Plugin\\).app" "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper \\(Plugin\\).app"
                     COMMAND cp -Rf "${obs_run_bin_out_dir_mac}/SOOPStudio Helper \\(Renderer\\).app" "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper \\(Renderer\\).app"
                     COMMAND cp -Rf "${obs_run_bin_out_dir_mac}/SOOPStudio Helper.app" "${_MAC_CUSTOM_BUILD_FRAMEWORKS_DIR}/SOOPStudio Helper.app"
                     COMMENT "Running Copy frameworks")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND rm -Rf "${LIBS_DIR}/QtCore.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtGui.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtUiTools.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtWidgets.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtOpenGLWidgets.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtOpenGL.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtSvg.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtSvgWidgets.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtDBus.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtNetwork.framework"
                     COMMAND rm -Rf "${LIBS_DIR}/QtXml.framework"
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
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtSvgWidgets.framework" "${LIBS_DIR}/QtSvgWidgets.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtDBus.framework" "${LIBS_DIR}/QtDBus.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtNetwork.framework" "${LIBS_DIR}/QtNetwork.framework"
                     COMMAND cp -Rf "$ENV{QTDIR}/lib/QtXml.framework" "${LIBS_DIR}/QtXml.framework"
                     COMMENT "Running Copy Qt bin")

  add_custom_command(TARGET SoopStudio
                     PRE_BUILD
                     COMMAND for libfile in ${LIBS_DIR}/*.dylib\; do codesign -s \"Developer ID Application: AfreecaTVCoLtd \(K6NJ2QFV2P\)\" -f $libfile\; done
                     COMMENT "Running Force codesign OBS deps")


# mac - qt, obs build post 처리 작업 해야함.
  
endif()
