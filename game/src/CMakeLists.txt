cmake_minimum_required (VERSION 2.8)
project(tzod)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

# gtest
set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")

# glfw
set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
set(GLFW_USE_DWM_SWAP_INTERVAL ON CACHE BOOL "Set swap interval even when DWM compositing is enabled")

add_subdirectory(glfw)
add_subdirectory(lua)
add_subdirectory(pluto)
add_subdirectory(zlib)
add_subdirectory(oggvorbis)
add_subdirectory(utfcpp)
add_subdirectory(fs)
add_subdirectory(math)
add_subdirectory(video)
add_subdirectory(mf)
add_subdirectory(gc)
add_subdirectory(ui)
add_subdirectory(tank)
add_subdirectory(tzodmain)

add_subdirectory(gtest)
add_subdirectory(gc_tests)
