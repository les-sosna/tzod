cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(config
	inc/config/ConfigBase.h
	inc/config/ConfigCache.h

	ConfigBase.cpp
	ConfigCache.cpp
)

target_link_libraries(config PRIVATE
	# 3rd party
	lua
)

target_include_directories(config INTERFACE inc)
