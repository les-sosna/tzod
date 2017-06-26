cmake_minimum_required (VERSION 3.3)

add_library(editor
	inc/editor/Config.h
	inc/editor/Editor.h
	inc/editor/MapSettings.h
	inc/editor/detail/DefaultCamera.h
	inc/editor/detail/QuickActions.h

	GameClassVis.h
	PropertyList.h
#	ServiceEditor.h

	Config.cpp
	DefaultCamera.cpp
	Editor.cpp
	GameClassVis.cpp
	MapSettings.cpp
	PropertyList.cpp
	QuickActions.cpp
#	ServiceEditor.cpp

)

target_link_libraries(editor PRIVATE
	cbind
	ctx
	gc
	gclua
	gv
	loc
	lua
	luaetc
	ui
	render
)

target_include_directories(editor INTERFACE inc)
