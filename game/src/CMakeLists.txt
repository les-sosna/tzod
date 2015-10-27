cmake_minimum_required (VERSION 3.3)
project(tzod)

if(APPLE OR UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# gtest
set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

add_subdirectory(external)

add_subdirectory(loc)
add_subdirectory(app)
add_subdirectory(config)
add_subdirectory(fs)
add_subdirectory(math)
add_subdirectory(video)
add_subdirectory(mf)
add_subdirectory(gc)
add_subdirectory(gc_tests)
add_subdirectory(ai)
add_subdirectory(gclua)
add_subdirectory(render)
add_subdirectory(script)
add_subdirectory(libui)

find_package(OpenAL)
if(${OPENAL_FOUND})
	add_subdirectory(audio)
endif()

if(NOT IOS)
	add_subdirectory(shell)
	add_subdirectory(tank)
	add_subdirectory(tzodmain)
endif()
