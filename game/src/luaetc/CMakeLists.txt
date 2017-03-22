cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(luaetc LuaDeleter.cpp)

target_link_libraries(luaetc lua)

target_include_directories(luaetc INTERFACE inc)
