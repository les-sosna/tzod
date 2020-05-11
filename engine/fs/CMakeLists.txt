add_library(fs
	inc/fs/FileSystem.h
	inc/fs/StreamWrapper.h

	FileSystem.cpp
	StreamWrapper.cpp	
)

target_include_directories(fs INTERFACE inc)
set_target_properties (fs PROPERTIES FOLDER engine)