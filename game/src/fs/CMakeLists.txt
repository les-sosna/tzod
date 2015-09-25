cmake_minimum_required (VERSION 3.3)
project(FileSystem)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${Utf8cpp_SOURCE_DIR}
)

add_library(filesystem
	FileSystemImpl.cpp
)
