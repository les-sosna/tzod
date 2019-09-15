cmake_minimum_required (VERSION 3.3)

add_library(fsposix
	inc/fsposix/FileSystemPosix.h
	FileSystemPosix.cpp
)

target_link_libraries(fsposix
	PUBLIC fs
)

target_include_directories(fsposix INTERFACE inc)
set_target_properties (fsposix PROPERTIES FOLDER engine)