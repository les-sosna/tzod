cmake_minimum_required (VERSION 3.3)

add_library(plat INTERFACE)

target_link_libraries(plat
	INTERFACE math
)

target_include_directories(plat INTERFACE inc)
