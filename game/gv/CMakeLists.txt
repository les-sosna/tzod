cmake_minimum_required (VERSION 3.3)

add_library(gv
	inc/gv/Camera.h
	inc/gv/GameViewHarness.h
	inc/gv/ThemeManager.h

	Camera.cpp
	GameViewHarness.cpp
	ThemeManager.cpp
)

target_link_libraries(gv PRIVATE
	as
	ctx
	fs
	gc
	render
	video

	PUBLIC as
)

target_include_directories(gv INTERFACE inc)
set_target_properties(gv PROPERTIES FOLDER game)