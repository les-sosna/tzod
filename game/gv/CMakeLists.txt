cmake_minimum_required (VERSION 3.3)

add_library(gv
	inc/gv/Camera.h
	inc/gv/GameViewHarness.h

	Camera.cpp
	GameViewHarness.cpp
)

target_link_libraries(gv PRIVATE
	ctx
	gc
	render
	video
)

target_include_directories(gv INTERFACE inc)
set_target_properties(gv PROPERTIES FOLDER game)