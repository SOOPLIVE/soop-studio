cmake_minimum_required(VERSION 3.22...3.25)

include(../common_mac.cmake)

add_library(soop-filters MODULE)
add_library(SOOP::soop-filters ALIAS soop-filters)

target_sources(soop-filters PRIVATE soop-filters.cpp
                                    shader-filter.cpp)

# Add shaders to the list of sources
set(SHADERS data/shaders/vertex_shader.glsl
            data/shaders/fragment_shader.glsl)
set(LOCALES data/locale/en-US.ini)

# Copy shaders to build directory
foreach(shader ${SHADERS}) 
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${shader} ${CMAKE_CURRENT_BINARY_DIR}/${shader} COPYONLY)
endforeach()
# Copy locales to build directory
foreach(locale ${LOCALES}) 
    configure_file(${CMAKE_CURRENT_SOURCE_DIR}/${locale} ${CMAKE_CURRENT_BINARY_DIR}/${locale} COPYONLY)
endforeach()

target_include_directories(soop-filters PRIVATE "${CMAKE_CURRENT_SOURCE_DIR}/deps")
target_compile_options(soop-filters PRIVATE $<$<COMPILE_LANG_AND_ID:C,AppleClang,Clang>:-Wno-shorten-64-to-32>)
target_compile_features(soop-filters PRIVATE cxx_std_17)

target_link_libraries(soop-filters PRIVATE 
    $<IF:$<PLATFORM_ID:Windows>,
   ${LIBS_DIR}/obs.lib, 
   ${LIBS_DIR}/libobs.framework/Versions/A/libobs>
)


if(WIN32)
set_target_properties(soop-filters PROPERTIES 
					  FOLDER "plugins"
					  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/obs-plugins/64bit")
else()
set_target_properties_obs(soop-filters PROPERTIES FOLDER plugins/soop-filters PREFIX "")
endif()
