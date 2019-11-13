cmake_minimum_required (VERSION 3.3)

if((NOT IOS) AND (NOT WINRT) AND (NOT ANDROID))
	set(GLFW_BUILD_EXAMPLES OFF CACHE BOOL "Build the GLFW example programs")
	set(GLFW_BUILD_TESTS OFF CACHE BOOL "Build the GLFW test programs")
	set(GLFW_BUILD_DOCS OFF CACHE BOOL "Build the GLFW documentation")
	set(GLFW_INSTALL OFF CACHE BOOL "Generate installation target")
	add_subdirectory(glfw)

	set(gtest_force_shared_crt ON CACHE BOOL "Use shared (DLL) run-time lib even when Google Test is built as static lib.")
	set(BUILD_SHARED_LIBS OFF CACHE BOOL "Build shared libraries")
	add_subdirectory(gtest)
	target_compile_definitions(gtest PUBLIC -DGTEST_LANG_CXX11 -DGTEST_HAS_TR1_TUPLE=0)

	set_target_properties(
		glfw
		gtest
		gtest_main
	PROPERTIES FOLDER external)
endif()

add_subdirectory(lua)
#if(WITH_SOUND)
#	add_subdirectory(oggvorbis)
#endif()
add_subdirectory(pluto)
add_subdirectory(utfcpp)
#add_subdirectory(zlib)

set_target_properties(
	lua
	pluto
#	zlib
PROPERTIES FOLDER external)
