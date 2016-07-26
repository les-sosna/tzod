cmake_minimum_required (VERSION 3.3)
project(LibUI)

if(APPLE OR UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
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
	DataSource.cpp
	Dialog.cpp
	Edit.cpp
	GuiManager.cpp
	InputContext.cpp
	LayoutContext.cpp
	List.cpp
	ListBase.cpp
	ListBox.cpp
	Scroll.cpp
	ScrollView.cpp
	StackLayout.cpp
	Rectangle.cpp
	Text.cpp
	Window.cpp

	inc/ui/Button.h
	inc/ui/Clipboard.h
	inc/ui/Combo.h
	inc/ui/Console.h
	inc/ui/ConsoleBuffer.h
	inc/ui/DataSource.h
	inc/ui/DataSourceAdapters.h
	inc/ui/Dialog.h
	inc/ui/Edit.h
	inc/ui/GuiManager.h
	inc/ui/InputContext.h
	inc/ui/Keys.h
	inc/ui/LayoutContext.h
	inc/ui/List.h
	inc/ui/ListBase.h
	inc/ui/ListBox.h
	inc/ui/Pointers.h
	inc/ui/Rectangle.h
	inc/ui/Scroll.h
	inc/ui/ScrollView.h
	inc/ui/StackLayout.h
	inc/ui/StateContext.h
	inc/ui/Text.h
	inc/ui/UIInput.h
	inc/ui/Window.h
)

target_link_libraries(ui
	video
)
