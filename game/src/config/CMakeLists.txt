cmake_minimum_required (VERSION 3.3)
project(Config)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	# 3rd party
	${Lua_SOURCE_DIR}/src
)

add_library(config
	ConfigBase.cpp
)

target_link_libraries(config
	# 3rd party
	lua
)
