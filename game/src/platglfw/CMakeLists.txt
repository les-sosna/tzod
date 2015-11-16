cmake_minimum_required (VERSION 3.3)
project(PlatGLFW)

include_directories(
	${App_SOURCE_DIR}/inc
	${LibUI_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
	${Video_SOURCE_DIR}/inc

	# 3rd party
	${GLFW_SOURCE_DIR}/include
)

add_library(platglfw
	GlfwAppWindow.cpp
	GlfwKeys.cpp
	GlfwPlatform.cpp
	Timer.cpp
)

target_link_libraries(platglfw
	ui
	glfw
	${GLFW_LIBRARIES}
)
