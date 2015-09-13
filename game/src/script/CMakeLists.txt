cmake_minimum_required (VERSION 2.8)
project(Script)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${GC_SOURCE_DIR}/inc
	${GCLua_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc

	# 3rd party
	${Lua_SOURCE_DIR}/src
	${Pluto_SOURCE_DIR}
)

add_library(script
	script.cpp
	ScriptHarness.cpp
	sTrigger.cpp
	sRigidBodyStatic.cpp
	sPickup.cpp
	sPlayer.cpp
)

target_link_libraries(script
	gc
	gclua
	filesystem

	# 3rd party
	lua
	pluto
)
