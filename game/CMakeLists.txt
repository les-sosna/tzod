cmake_minimum_required (VERSION 3.6)
project(tzod VERSION 1.52)

if(APPLE OR UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
endif()

if(WIN32)
	set_property(DIRECTORY ${CMAKE_CURRENT_SOURCE_DIR} PROPERTY VS_STARTUP_PROJECT tzodmain)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std:c++latest")
	set(WITH_SOUND 1)
else()
	find_package(OpenAL)
	set(WITH_SOUND ${OPENAL_FOUND})
endif()

# libs
add_subdirectory(loc)
add_subdirectory(mf)
add_subdirectory(gc)
add_subdirectory(ai)
add_subdirectory(gclua)
add_subdirectory(script)
add_subdirectory(ctx)
add_subdirectory(as)
add_subdirectory(render)
add_subdirectory(gv)
add_subdirectory(editor)
add_subdirectory(shell)
if(WITH_SOUND)
	add_subdirectory(audio)
endif()

# top level lib - the only one the application links with
add_subdirectory(app)

# desktop applications
if((NOT IOS) AND (NOT WINRT) AND (NOT ANDROID))
	add_subdirectory(tzodmain)
	add_subdirectory(gc_tests)
endif()
