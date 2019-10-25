cmake_minimum_required (VERSION 3.3)

add_library(gclua
#	lGame.cpp
	lObject.cpp
	lObjUtil.cpp
	lWorld.cpp
)

target_link_libraries(gclua PRIVATE
	gc

	# 3rd party
	lua
)

target_include_directories(gclua INTERFACE inc)
set_target_properties(gclua PROPERTIES FOLDER game)