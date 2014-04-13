cmake_minimum_required (VERSION 2.8)
project(FileSystem)

include_directories(
	# 3rd party
	${Utf8cpp_SOURCE_DIR}
)

add_library(libFileSystem
	FileSystemImpl.cpp
)
