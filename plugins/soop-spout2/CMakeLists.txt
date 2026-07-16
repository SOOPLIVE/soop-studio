cmake_minimum_required(VERSION 3.22...3.25)

add_library(soop-spout2 MODULE)
add_library(SOOP::soop-spout2 ALIAS soop-spout2)

set_target_properties(soop-spout2 PROPERTIES AUTOMOC OFF)
set_target_properties(soop-spout2 PROPERTIES AUTOUIC OFF)
set_target_properties(soop-spout2 PROPERTIES AUTORCC OFF)

include_directories(
  ${LIBOBS_DIR}
  Spout2/SPOUTSDK/SpoutLibrary
  Spout2/SPOUTSDK/SpoutDirectX/SpoutDX
)

target_link_libraries(soop-spout2
   ${LIBS_DIR}/obs.lib
   ${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/SpoutDX.lib
)

target_sources(soop-spout2 PRIVATE soop-spout2-plugin.cpp)


if(MSVC)
    target_compile_definitions(soop-spout2 PRIVATE UNICODE _UNICODE)
endif()

set_target_properties(soop-spout2 PROPERTIES 
					  FOLDER "plugins"
					  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/obs-plugins/64bit")

# copy spout2 bin
add_custom_command(TARGET soop-spout2 POST_BUILD
		COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/Spout.dll"
			"${CMAKE_BINARY_DIR}/$<CONFIG>/obs-plugins/64bit/Spout.dll"
		COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/SpoutLibrary.dll"
			"${CMAKE_BINARY_DIR}/$<CONFIG>/obs-plugins/64bit/SpoutLibrary.dll"
		COMMAND ${CMAKE_COMMAND} -E copy
			"${CMAKE_CURRENT_SOURCE_DIR}/bin/x64/SpoutDX.dll"
			"${CMAKE_BINARY_DIR}/$<CONFIG>/obs-plugins/64bit/SpoutDX.dll"
	)