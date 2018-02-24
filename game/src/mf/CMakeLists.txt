cmake_minimum_required (VERSION 3.3)

add_library(mapfile
	MapFile.cpp
)

target_link_libraries(mapfile PRIVATE
	filesystem
)

target_include_directories(mapfile INTERFACE inc)
