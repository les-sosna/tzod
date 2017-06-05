cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(video_SOURCES
	inc/video/ImageLoader.h
	inc/video/RenderBase.h
	inc/video/RenderContext.h
	inc/video/TextureManager.h

	ImageLoader.cpp
	RenderContext.cpp
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
		inc/video/RenderD3D11.h
		RenderD3D11.cpp
	)
else()
	list(APPEND video_SOURCES
		inc/video/RenderOpenGL.h
		RenderOpenGL.cpp
	)
endif()

add_library(video ${video_SOURCES})

target_link_libraries(video PRIVATE
	filesystem
	lua
	luaetc
	math
)

target_include_directories(video PRIVATE
	# 3rd party
	${GLFW_SOURCE_DIR}/include
)

target_include_directories(video INTERFACE inc)
