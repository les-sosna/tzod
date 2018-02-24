cmake_minimum_required (VERSION 3.3)

add_library(config
	inc/config/ConfigBase.h
	inc/config/ConfigCache.h

	ConfigBase.cpp
	ConfigCache.cpp
)

target_link_libraries(config PRIVATE
	filesystem
	luaetc
	lua #external
)

target_include_directories(config INTERFACE inc)
