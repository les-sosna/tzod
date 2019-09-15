cmake_minimum_required (VERSION 3.3)

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
	fs

	# 3rd party
	lua
	pluto

	PUBLIC luaetc
)

target_include_directories(script INTERFACE inc)
set_target_properties(script PROPERTIES FOLDER tzod)