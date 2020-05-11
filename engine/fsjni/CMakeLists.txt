add_library(fsjni
	inc/fsjni/FileSystemJni.h
	FileSystemJni.cpp
)

target_link_libraries(fsjni
PUBLIC
	fs

PRIVATE
	android
)

target_include_directories(fsjni INTERFACE inc)
set_target_properties (fsjni PROPERTIES FOLDER engine)