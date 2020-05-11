add_library(fsmem
	inc/fsmem/FileSystemMemory.h
	FileSystemMemory.cpp
)

target_link_libraries(fsmem
	PUBLIC fs
)

target_include_directories(fsmem INTERFACE inc)
set_target_properties (fsmem PROPERTIES FOLDER engine)