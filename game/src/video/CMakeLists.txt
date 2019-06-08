cmake_minimum_required (VERSION 3.3)

set(video_SOURCES
	inc/video/EditableImage.h
	inc/video/ImageLoader.h
	inc/video/RenderBase.h
	inc/video/RenderContext.h
	inc/video/TextureManager.h
	inc/video/TexturePackage.h

	AtlasPacker.cpp
	AtlasPacker.h
	EditableImage.cpp
	ImageLoader.cpp
	RenderContext.cpp
	TextureManager.cpp
	TexturePackage.cpp
)

if(IOS OR ANDROID)
	list(APPEND video_SOURCES
		inc/video/RenderOpenGL.h
		GLESProgram.cpp
		RenderGLES2.cpp
	)
elseif(WINRT)
	list(APPEND video_SOURCES
		inc/video/RenderD3D11.h
		inc/video/SwapChainResources.h
		RenderD3D11.cpp
		SwapChainResources.cpp
	)
else()
	list(APPEND video_SOURCES
		inc/video/RenderOpenGL.h
		RenderOpenGL.cpp
	)
endif()

add_library(video ${video_SOURCES})

target_link_libraries(video PRIVATE
	fs
	lua
	luaetc
	math
)

target_include_directories(video PRIVATE
	# 3rd party
	${GLFW_SOURCE_DIR}/include
)

target_include_directories(video INTERFACE inc)
