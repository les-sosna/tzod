cmake_minimum_required (VERSION 3.3)
project(LuaEtc)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	# 3rd party
	${Lua_SOURCE_DIR}/src
)

add_library(luaetc
	LuaDeleter.cpp
)

target_link_libraries(luaetc
	lua
)
