cmake_minimum_required (VERSION 3.3)

if(APPLE OR UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

add_library(ui
	Button.cpp
	Combo.cpp
	Console.cpp
	ConsoleBuffer.cpp
	DataSource.cpp
	Dialog.cpp
	Edit.cpp
	EditableText.cpp
	GuiManager.cpp
	InputContext.cpp
	LayoutContext.cpp
	List.cpp
	ListBase.cpp
	ListBox.cpp
	ListSelectionBinding.cpp
	MultiColumnListItem.cpp
	Navigation.cpp
	Rating.cpp
	Rectangle.cpp
	ScrollBar.cpp
	ScrollView.cpp
	StackLayout.cpp
	Text.cpp
	Texture.cpp
	Window.cpp

	inc/ui/Button.h
	inc/ui/Clipboard.h
	inc/ui/Combo.h
	inc/ui/Console.h
	inc/ui/ConsoleBuffer.h
	inc/ui/DataContext.h
	inc/ui/DataSource.h
	inc/ui/DataSourceAdapters.h
	inc/ui/Dialog.h
	inc/ui/Edit.h
	inc/ui/EditableText.h
	inc/ui/GuiManager.h
	inc/ui/InputContext.h
	inc/ui/Keys.h
	inc/ui/LayoutContext.h
	inc/ui/List.h
	inc/ui/ListBase.h
	inc/ui/ListBox.h
	inc/ui/ListSelectionBinding.h
	inc/ui/MultiColumnListItem.h
	inc/ui/Navigation.h
	inc/ui/Pointers.h
	inc/ui/Rating.h
	inc/ui/Rectangle.h
	inc/ui/ScrollBar.h
	inc/ui/ScrollView.h
	inc/ui/StackLayout.h
	inc/ui/StateContext.h
	inc/ui/Text.h
	inc/ui/Texture.h
	inc/ui/UIInput.h
	inc/ui/Window.h
)

target_link_libraries(ui PUBLIC
	math
	video
)

target_include_directories(ui INTERFACE inc)
