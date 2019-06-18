cmake_minimum_required (VERSION 3.3)

add_library(fsmem
	inc/fsmem/FileSystemMemory.h
	FileSystemMemory.cpp
)

target_link_libraries(fsmem
	PUBLIC fs
)

target_include_directories(fsmem INTERFACE inc)
