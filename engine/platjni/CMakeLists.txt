cmake_minimum_required(VERSION 3.4.1)

add_library(platjni
	inc/platjni/JniAppWindow.h
	inc/platjni/JniClipboard.h
	inc/platjni/JniConsoleLog.h
	inc/platjni/JniInput.h

	JniAppWindow.cpp
	JniClipboard.cpp
	JniConsoleLog.cpp
	JniInput.cpp
)


target_link_libraries(platjni
PUBLIC
	plat

PRIVATE
	android
	video
)

target_include_directories(platjni INTERFACE inc)
set_target_properties(platjni PROPERTIES FOLDER engine)