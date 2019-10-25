cmake_minimum_required (VERSION 3.3)

add_library(editor
	inc/editor/Config.h
	inc/editor/EditorMain.h
	inc/editor/EditorWorldView.h
	inc/editor/MapSettings.h
	inc/editor/detail/DefaultCamera.h
	inc/editor/detail/QuickActions.h

	GameClassVis.h
	MapList.h
	PropertyList.h
#	ServiceEditor.h

	Config.cpp
	DefaultCamera.cpp
	EditorMain.cpp
	EditorWorldView.cpp
	GameClassVis.cpp
	MapList.cpp
	MapSettings.cpp
	PropertyList.cpp
	QuickActions.cpp
#	ServiceEditor.cpp

)

target_link_libraries(editor PRIVATE
	cbind
	ctx
	fs
	gc
	gclua
	gv
	loc
	lua
	luaetc
	plat
	render
	ui
)

target_include_directories(editor INTERFACE inc)
set_target_properties(editor PROPERTIES FOLDER game)