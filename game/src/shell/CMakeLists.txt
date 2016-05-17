cmake_minimum_required (VERSION 3.3)
project(Shell)

include_directories(
	${AS_SOURCE_DIR}/inc
	${Config_SOURCE_DIR}/inc
	${Ctx_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${GC_SOURCE_DIR}/inc
	${GCLua_SOURCE_DIR}/inc
	${GV_SOURCE_DIR}/inc
	${LibUI_SOURCE_DIR}/inc
	${Loc_SOURCE_DIR}/inc
	${MapFile_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
	${Render_SOURCE_DIR}/inc
	${Script_SOURCE_DIR}/inc
	${Video_SOURCE_DIR}/inc

	# 3rd party
	${Lua_SOURCE_DIR}/src
)

add_library(shell
	gui.cpp
	Campaign.cpp
	Config.cpp
	ConfigConsoleHistory.cpp
	Controller.cpp
	DefaultCamera.cpp
	Desktop.cpp
	Editor.cpp
	Game.cpp
	GetFileName.cpp
	InputManager.cpp
	KeyMapper.cpp
	MainMenu.cpp
	MapList.cpp
	MapSettings.cpp
	MessageArea.cpp
	Network.cpp
	NewMap.cpp
	Profiler.cpp
	PropertyList.cpp
	ScoreTable.cpp
	ServiceEditor.cpp
	Settings.cpp
	Widgets.cpp
)

target_link_libraries(shell
	app
	config
	loc
	gc
	gclua
	gv
	mapfile
	render
	ui
	lua
)

