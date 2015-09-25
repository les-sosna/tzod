cmake_minimum_required (VERSION 3.3)
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

set(video_SOURCES
	DrawingContext.cpp
	ImageLoader.cpp
#	RenderDirect3D.cpp
	TextureManager.cpp
)

if(IOS)
	list(APPEND video_SOURCES
		GLESProgram.cpp
		RenderGLES2.cpp
	)
else()
	list(APPEND video_SOURCES RenderOpenGL.cpp)
endif()

add_library(video ${video_SOURCES})

target_link_libraries(video
	filesystem
	lua
	${GLFW_LIBRARIES}
)
