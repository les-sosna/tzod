cmake_minimum_required (VERSION 2.8)
project(MapFile)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	include
	${FileSystem_SOURCE_DIR}/include
)

add_library(mapfile
	MapFile.cpp
)

target_link_libraries(mapfile 
	filesystem
)
