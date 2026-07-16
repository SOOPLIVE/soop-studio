cmake_minimum_required(VERSION 3.22...3.25)

add_library(soop-streamdeck MODULE)
add_library(SOOP::soop-streamdeck ALIAS soop-streamdeck)

file(READ ${CMAKE_SOURCE_DIR}/obs-studio/buildspec.json BUILD_SPEC_CONTENTS)
string(JSON DEPS_VERSION GET ${BUILD_SPEC_CONTENTS} "dependencies" "prebuilt" "version")

set(OBS_DOT_DEPS_DIR "${CMAKE_SOURCE_DIR}/obs-studio/.deps/obs-deps-${DEPS_VERSION}-x64/include")

message(STATUS ${OBS_DOT_DEPS_DIR})


include_directories(
  ${LIBOBS_DIR}
  ${OBS_DEPS_DIR}
  ${OBS_DOT_DEPS_DIR}
)

target_include_directories(soop-streamdeck PRIVATE ${OBS_DOT_DEPS_DIR})
target_include_directories(soop-streamdeck PRIVATE ${OBS_DEPS_DIR})
target_include_directories(soop-streamdeck PRIVATE ${OBS_FEUI_DIR})

target_compile_definitions(
  soop-streamdeck PRIVATE ASIO_STANDALONE $<$<BOOL:${PLUGIN_TESTS}>:PLUGIN_TESTS>
                        $<$<PLATFORM_ID:Windows>:_WEBSOCKETPP_CPP11_STL_> $<$<PLATFORM_ID:Windows>:_WIN32_WINNT=0x0603>)

target_link_libraries(soop-streamdeck
   ${LIBS_DIR}/obs.lib
   ${LIBS_DIR}/obs-frontend-api.lib
)

target_sources(soop-streamdeck PRIVATE 
	soop-streamdeck.h
	soop-streamdeck.cpp
	WebSocketServer.h
	WebSocketServer.cpp
	EventHandler.h
	EventHandler.cpp)

set_target_properties(soop-streamdeck PROPERTIES 
					  FOLDER "plugins"
					  LIBRARY_OUTPUT_DIRECTORY "${CMAKE_BINARY_DIR}/$<CONFIG>/obs-plugins/64bit")
