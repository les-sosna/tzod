cmake_minimum_required (VERSION 2.8)
project(UI)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	..
	${GLFW_SOURCE_DIR}/include
)

add_library(libUI
	Button.cpp
	Combo.cpp
	Console.cpp
	ConsoleBuffer.cpp
	Dialog.cpp
	Edit.cpp
	GuiManager.cpp
	List.cpp
	ListBase.cpp
	Scroll.cpp
	Text.cpp
	Window.cpp
)
