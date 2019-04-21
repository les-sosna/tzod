cmake_minimum_required (VERSION 3.3)

if(WIN32)
	set(PLATFORM_SRC PlatFoldersWin.cpp)
elseif(APPLE)
	set(PLATFORM_SRC PlatFoldersStub.cpp)
elseif(UNIX)
	set(PLATFORM_SRC PlatFoldersStub.cpp)
else()
	message(FATAL_ERROR "Unknown platform")
endif()


add_library(platglfw
	inc/platglfw/GlfwAppWindow.h
	inc/platglfw/GlfwKeys.h
	inc/platglfw/GlfwPlatform.h
	inc/platglfw/Timer.h

	GlfwAppWindow.cpp
	GlfwKeys.cpp
	GlfwPlatform.cpp
	Timer.cpp

	${PLATFORM_SRC}
)

find_package(OpenGL REQUIRED)

target_link_libraries(platglfw
	PRIVATE video glfw utf8cpp ${OPENGL_gl_LIBRARY}

	PUBLIC plat
)

target_include_directories(platglfw PRIVATE
	${GLFW_SOURCE_DIR}/include
)

target_include_directories(platglfw INTERFACE inc)
