cmake_minimum_required (VERSION 3.3)

add_library(plat
	inc/plat/AppWindow.h
	inc/plat/Clipboard.h
	inc/plat/ConsoleBuffer.h
	inc/plat/Folders.h
	inc/plat/Input.h
	inc/plat/Keys.h

	ConsoleBuffer.cpp
)

target_link_libraries(plat
	INTERFACE math
)

target_include_directories(plat INTERFACE inc)
