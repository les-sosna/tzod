cmake_minimum_required (VERSION 3.3)

set(FileSystem_SOURCES FileSystem.cpp StreamWrapper.cpp)
if(WIN32)
	list(APPEND FileSystem_SOURCES FileSystemWin32.cpp)
else()
	list(APPEND FileSystem_SOURCES FileSystemPosix.cpp)
endif()

add_library(filesystem
	inc/fs/FileSystem.h
	inc/fs/StreamWrapper.h
	
	${FileSystem_SOURCES}
)

target_link_libraries(filesystem PRIVATE utf8cpp)

target_include_directories(filesystem INTERFACE inc)
