
cmake_minimum_required(VERSION 3.22...3.25)


project(SoopStudio LANGUAGES CXX C)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)


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
find_package(Qt6SvgWidgets REQUIRED)
find_package(Qt6Network REQUIRED)
find_package(Qt6Xml REQUIRED)
find_package(Qt6 REQUIRED COMPONENTS SvgWidgets)

include(cmake/mac/findPackage.cmake)



#=================== INCLUSION OF Project Files ====================#
set(SOURCE_DIR "${CMAKE_SOURCE_DIR}/src")
set(FORMS_DIR "${CMAKE_SOURCE_DIR}/src/ui-forms")
set(TOOLBAR_FORMS_DIR "${CMAKE_SOURCE_DIR}/src/ui-forms/toolbars")
set(ASSETS_DIR "${CMAKE_SOURCE_DIR}/assets")
set(LIBS_DIR "${CMAKE_SOURCE_DIR}/lib")
set(CSS_DIR "${CMAKE_SOURCE_DIR}/src/ui-forms/qss")
set(LOCALE_DATA_DIR "${CMAKE_SOURCE_DIR}/localeData")
set(LIBOBS_DIR "${CMAKE_SOURCE_DIR}/obs-studio/libobs")
set(OBS_UI_DIR "${CMAKE_SOURCE_DIR}/obs-studio-ui")
set(OBS_FEUI_DIR "${CMAKE_SOURCE_DIR}/obs-studio/UI/obs-frontend-api")
set(OBS_DEPS_DIR "${CMAKE_SOURCE_DIR}/obs-studio/deps")
set(OBS_JSON_DEPS_DIR "${CMAKE_SOURCE_DIR}/obs-studio/deps/json11")
set(OBS_SHARED_DIR "${CMAKE_SOURCE_DIR}/shared")

include(cmake/win/setPath.cmake)
include(cmake/mac/setPath.cmake)



file(GLOB_RECURSE ALL_SRC_LIST 
     "${SOURCE_DIR}/*.cpp*"
     "${SOURCE_DIR}/*.h*"
     "${SOURCE_DIR}/*.hpp*"
     
     "${OBS_UI_DIR}/*.cpp*"
     "${OBS_UI_DIR}/*.h*"
     "${OBS_UI_DIR}/*.hpp*"
     
     "${OBS_SHARED_DIR}/*.cpp*"
     "${OBS_SHARED_DIR}/*.h*"
     "${OBS_SHARED_DIR}/*.hpp*")


file(GLOB_RECURSE ALL_FORM_LIST 
     "${FORMS_DIR}/*.ui*"
	 "${TOOLBAR_FORMS_DIR}/*.ui*")

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

include(cmake/mac/setResource.cmake)



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

foreach(_source IN ITEMS ${ALL_MM_FILE_LIST})
    get_filename_component(_source_path "${_source}" PATH)
    string(REPLACE "${CMAKE_SOURCE_DIR}" "" _group_path "${_source_path}")
    string(REPLACE "/" "\\" _group_path "${_group_path}")
    source_group("${_group_path}" FILES "${_source}")
endforeach()

include_directories(
	${FORMS_DIR}
	${TOOLBAR_FORMS_DIR}
	${SOURCE_DIR}
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
set(CMAKE_AUTOUIC_SEARCH_PATHS ${CMAKE_AUTOUIC_SEARCH_PATHS} ${TOOLBAR_FORMS_DIR})


# Add the executable
include(cmake/win/setBuild.cmake)
include(cmake/mac/setBuild.cmake)
 

# Add the target includes for SoopStudio
target_include_directories(SoopStudio PRIVATE ${FORMS_DIR})
target_include_directories(SoopStudio PRIVATE ${TOOLBAR_FORMS_DIR})
target_include_directories(SoopStudio PRIVATE ${SOURCE_DIR})
target_include_directories(SoopStudio PRIVATE ${PLATFORM_DIR})

target_include_directories(SoopStudio PRIVATE ${OBS_DEPS_DIR})
target_include_directories(SoopStudio PRIVATE ${OBS_UI_DIR})
target_include_directories(SoopStudio PRIVATE ${OBS_FEUI_DIR})
target_include_directories(SoopStudio PRIVATE ${OBS_JSON_DEPS_DIR})

target_include_directories(SoopStudio PRIVATE "${OBS_SHARED_DIR}/bpm")
target_include_directories(SoopStudio PRIVATE "${OBS_SHARED_DIR}/properties-view")
target_include_directories(SoopStudio PRIVATE "${OBS_SHARED_DIR}/qt/icon-label")
target_include_directories(SoopStudio PRIVATE "${OBS_SHARED_DIR}/qt/plain-text-edit")
target_include_directories(SoopStudio PRIVATE "${OBS_SHARED_DIR}/qt/slider-ignorewheel")
target_include_directories(SoopStudio PRIVATE "${OBS_SHARED_DIR}/qt/vertical-scroll-area")
target_include_directories(SoopStudio PRIVATE "${OBS_SHARED_DIR}/qt/wrappers")

#Davin Need Test
add_subdirectory("${CMAKE_SOURCE_DIR}/obs-studio/deps/json11" "${CMAKE_BINARY_DIR}/deps/json11")
#

#Add Plugins
add_subdirectory(plugins)
#



#---------- obs-browser setting  ----------#
add_library(browser-panels INTERFACE)
add_library(OBS::browser-panels ALIAS browser-panels)

#target_sources(browser-panels INTERFACE panel/browser-panel.hpp)

target_include_directories(browser-panels INTERFACE "${CMAKE_SOURCE_DIR}/obs-studio/plugins/obs-browser/panel")

target_compile_definitions(browser-panels INTERFACE BROWSER_AVAILABLE)

target_link_libraries(SoopStudio PRIVATE OBS::browser-panels)



#----------------------- OBS DEPS -----------------------#
include(cmake/win/obs.cmake)
include(cmake/mac/obs.cmake)
target_include_directories(SoopStudio PRIVATE ${OBS_EX_DEPS_DIR})


#===================== LINKING LIBRARIES =======================#

target_link_libraries(SoopStudio
	PRIVATE Qt6::UiTools
  OBS::json11
  Qt6::Svg
  Qt6::Network
  Qt6::Xml
  Qt6::SvgWidgets
  $<IF:$<PLATFORM_ID:Windows>,
   ${LIBS_DIR}/obs.lib, 
   ${LIBS_DIR}/libobs.framework/Versions/A/libobs>
  $<IF:$<PLATFORM_ID:Windows>,
   ${LIBS_DIR}/obs-frontend-api.lib, 
   ${LIBS_DIR}/obs-frontend-api.dylib>
)


#===================== IDE PROJECT SETTINGS =======================#
#-------------------------       VC       -------------------------#
include(cmake/win/main.cmake)
include(cmake/win/copyMapFiles.cmake)




#-------------------------     XCODE     -------------------------#
include(cmake/mac/main.cmake)


#-------------------------     StyleSheet     -------------------------#
target_sources(SoopStudio PRIVATE src/ui-forms/qss/Black.qss)