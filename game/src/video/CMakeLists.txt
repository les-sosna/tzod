cmake_minimum_required (VERSION 2.8)
project(Video)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${FileSystem_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc

	# 3rd party
	${GLFW_SOURCE_DIR}/include
	${Lua_SOURCE_DIR}/src
)

add_library(video
	DrawingContext.cpp
	ImageLoader.cpp
#	RenderDirect3D.cpp
	RenderOpenGL.cpp
	TextureManager.cpp
)

target_link_libraries(video 
	filesystem
	lua
	${GLFW_LIBRARIES}
)
