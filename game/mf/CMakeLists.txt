add_library(mapfile
	MapFile.cpp
)

target_link_libraries(mapfile PRIVATE
	fs
)

target_include_directories(mapfile INTERFACE inc)
set_target_properties(mapfile PROPERTIES FOLDER game)