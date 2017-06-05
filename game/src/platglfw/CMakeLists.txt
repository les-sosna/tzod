cmake_minimum_required (VERSION 3.3)

add_library(platglfw
	GlfwAppWindow.cpp
	GlfwKeys.cpp
	GlfwPlatform.cpp
	Timer.cpp
)

target_link_libraries(platglfw PRIVATE
	glfw
	opengl32
	
	PUBLIC ui
)

target_include_directories(platglfw PRIVATE
	${GLFW_SOURCE_DIR}/include
)

target_include_directories(platglfw INTERFACE inc)
