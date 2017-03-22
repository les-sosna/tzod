cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(gv
	inc/gv/Camera.h
	inc/gv/GameViewHarness.h
	inc/gv/ThemeManager.h

	Camera.cpp
	GameViewHarness.cpp
	ThemeManager.cpp
)

target_link_libraries(gv
	as
	ctx
	filesystem
	gc
	render
	video
)

target_include_directories(gv INTERFACE inc)
