cmake_minimum_required (VERSION 3.3)

add_library(fs
	inc/fs/FileSystem.h
	inc/fs/StreamWrapper.h

	FileSystem.cpp
	StreamWrapper.cpp	
)

target_include_directories(fs INTERFACE inc)
