cmake_minimum_required (VERSION 2.8)
project(Config)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${FileSystem_SOURCE_DIR}/inc

	# 3rd party
	${Lua_SOURCE_DIR}/src
)

add_library(config
	ConfigBase.cpp
)

target_link_libraries(config 
	filesystem

	# 3rd party
	lua
)
