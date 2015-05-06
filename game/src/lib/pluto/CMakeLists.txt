cmake_minimum_required (VERSION 2.8)
project(Pluto)
include_directories(${Lua_SOURCE_DIR}/src)
add_library(pluto
	pluto.c
)
