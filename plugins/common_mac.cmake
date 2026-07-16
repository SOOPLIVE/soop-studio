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

# target_install_resources: Helper function to add resources into bundle
function(target_install_resources target)
  message(DEBUG "Installing resources for target ${target}...")
  if(EXISTS "${CMAKE_CURRENT_SOURCE_DIR}/data")
    file(GLOB_RECURSE data_files "${CMAKE_CURRENT_SOURCE_DIR}/data/*")
    foreach(data_file IN LISTS data_files)
      cmake_path(RELATIVE_PATH data_file BASE_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}/data/" OUTPUT_VARIABLE
                 relative_path)
      cmake_path(GET relative_path PARENT_PATH relative_path)
      target_sources(${target} PRIVATE "${data_file}")
      set_property(SOURCE "${data_file}" PROPERTY MACOSX_PACKAGE_LOCATION "Resources/${relative_path}")
      source_group("Resources/${relative_path}" FILES "${data_file}")
    endforeach()
  endif()
endfunction()

# set_target_properties_obs: Set target properties for use in obs-studio
function(set_target_properties_obs target)
  set(options "")
  set(oneValueArgs "")
  set(multiValueArgs PROPERTIES)
  cmake_parse_arguments(PARSE_ARGV 0 _STPO "${options}" "${oneValueArgs}" "${multiValueArgs}")

  message(DEBUG "Setting additional properties for target ${target}...")

  while(_STPO_PROPERTIES)
    list(POP_FRONT _STPO_PROPERTIES key value)
    set_property(TARGET ${target} PROPERTY ${key} "${value}")
  endwhile()
  get_target_property(target_type ${target} TYPE)

  string(TIMESTAMP CURRENT_YEAR "%Y")

  # Target is a GUI or CLI application
  if(target_type STREQUAL MODULE_LIBRARY)

    set_target_properties(${target} PROPERTIES BUNDLE TRUE BUNDLE_EXTENSION plugin)

      # cmake-format: off
      set_target_xcode_properties(
        ${target}
        PROPERTIES PRODUCT_NAME ${target}
                   PRODUCT_BUNDLE_IDENTIFIER com.soop.studio.${target}
                   CURRENT_PROJECT_VERSION ${OBS_BUILD_NUMBER}
                   MARKETING_VERSION ${OBS_VERSION_CANONICAL}
                   GENERATE_INFOPLIST_FILE YES
                   INFOPLIST_KEY_CFBundleDisplayName ${target}
                   INFOPLIST_KEY_NSHumanReadableCopyright "(c) 2025-${CURRENT_YEAR} soop corp")
      # cmake-format: on

    set_property(GLOBAL APPEND PROPERTY OBS_MODULES_ENABLED ${target})
    set_property(GLOBAL APPEND PROPERTY _OBS_DEPENDENCIES ${target})
  endif()

  target_install_resources(${target})

  get_target_property(target_sources ${target} SOURCES)
  set(target_ui_files ${target_sources})
  list(FILTER target_ui_files INCLUDE REGEX ".+\\.(ui|qrc)")
  source_group(
    TREE "${CMAKE_CURRENT_SOURCE_DIR}"
    PREFIX "UI Files"
    FILES ${target_ui_files})

endfunction()