cmake_minimum_required (VERSION 2.8)
project(tzod)

if(APPLE OR UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# gtest
set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

add_subdirectory(lib)
add_subdirectory(ai)
add_subdirectory(fs)
add_subdirectory(math)
add_subdirectory(video)
add_subdirectory(mf)
add_subdirectory(gc)
add_subdirectory(gc_tests)
add_subdirectory(render)
add_subdirectory(ui)
add_subdirectory(tank)
add_subdirectory(tzodmain)
