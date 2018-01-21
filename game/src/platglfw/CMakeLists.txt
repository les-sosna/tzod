cmake_minimum_required (VERSION 3.3)

add_library(platglfw
	inc/platglfw/GlfwAppWindow.h
	inc/platglfw/GlfwKeys.h
	inc/platglfw/GlfwPlatform.h
	inc/platglfw/Timer.h

	GlfwAppWindow.cpp
	GlfwKeys.cpp
	GlfwPlatform.cpp
	Timer.cpp
)

find_package(OpenGL REQUIRED)

target_link_libraries(platglfw
	PRIVATE glfw ${OPENGL_gl_LIBRARY}

	PUBLIC plat ui
)

target_include_directories(platglfw PRIVATE
	${GLFW_SOURCE_DIR}/include
)

target_include_directories(platglfw INTERFACE inc)
