add_library(fswin
	inc/fswin/FileSystemWin32.h
	FileSystemWin32.cpp
)

target_link_libraries(fswin
	PUBLIC fs
	PRIVATE utf8cpp
)

target_include_directories(fswin INTERFACE inc)
set_target_properties (fswin PROPERTIES FOLDER engine)