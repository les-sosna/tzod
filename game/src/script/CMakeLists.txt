cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

add_library(script
	script.cpp
	ScriptHarness.cpp
	sTrigger.cpp
	sRigidBodyStatic.cpp
	sPickup.cpp
	sPlayer.cpp
)

target_link_libraries(script PRIVATE
	gc
	gclua
	filesystem

	# 3rd party
	lua
	pluto

	PUBLIC luaetc
)

target_include_directories(script INTERFACE inc)
