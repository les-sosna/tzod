cmake_minimum_required (VERSION 3.3)
project(Video)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${FileSystem_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
	${LuaEtc_SOURCE_DIR}/inc

	# 3rd party
	${GLFW_SOURCE_DIR}/include
	${Lua_SOURCE_DIR}/src
)

set(video_SOURCES
	inc/video/DrawingContext.h
	inc/video/ImageLoader.h
	inc/video/RenderBase.h
	inc/video/TextureManager.h

	DrawingContext.cpp
	ImageLoader.cpp
	TextureManager.cpp
)

if(IOS)
	list(APPEND video_SOURCES
		inc/video/RenderOpenGL.h
		GLESProgram.cpp
		RenderGLES2.cpp
	)
elseif(WINRT)
	list(APPEND video_SOURCES
		inc/video/RenderDirect3D.h
		inc/video/RenderD3D11.h
		RenderD3D11.cpp
#		RenderDirect3D.cpp
	)
else()
	list(APPEND video_SOURCES
		inc/video/RenderOpenGL.h
		RenderOpenGL.cpp
	)
endif()

add_library(video ${video_SOURCES})

target_link_libraries(video
	filesystem
	lua
	luaetc
	${GLFW_LIBRARIES}
)
