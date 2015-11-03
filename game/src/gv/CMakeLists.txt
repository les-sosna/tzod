cmake_minimum_required (VERSION 3.3)
project(GV)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${App_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${GC_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
	${Render_SOURCE_DIR}/inc
	${Script_SOURCE_DIR}/inc
	${Video_SOURCE_DIR}/inc
)

add_library(gv
	Camera.cpp
	GameView.cpp
	GameViewHarness.cpp
	ThemeManager.cpp
)

target_link_libraries(gv
	app
	filesystem
	gc
	render
	video
)
