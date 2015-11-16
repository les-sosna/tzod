cmake_minimum_required (VERSION 3.3)
project(LibUI)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${Math_SOURCE_DIR}/inc
	${Video_SOURCE_DIR}/inc
)

add_library(ui
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
	WindowPtr.cpp
)

target_link_libraries(ui
	video
)
