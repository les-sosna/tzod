cmake_minimum_required (VERSION 3.3)

add_library(platglfw
	GlfwAppWindow.cpp
	GlfwKeys.cpp
	GlfwPlatform.cpp
	Timer.cpp
)

find_package(OpenGL REQUIRED)

target_link_libraries(platglfw
	PRIVATE glfw ${OPENGL_gl_LIBRARY}

	PUBLIC ui
)

target_include_directories(platglfw PRIVATE
	${GLFW_SOURCE_DIR}/include
)

target_include_directories(platglfw INTERFACE inc)
