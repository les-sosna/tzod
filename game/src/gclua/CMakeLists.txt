cmake_minimum_required (VERSION 3.3)
project(GCLua)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${GC_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc

	# 3rd party
	${Lua_SOURCE_DIR}/src
)

add_library(gclua
#	lGame.cpp
	lObject.cpp
	lObjUtil.cpp
	lWorld.cpp
)

target_link_libraries(gclua
	gc

	# 3rd party
	lua
)
