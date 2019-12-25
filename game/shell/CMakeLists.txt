cmake_minimum_required (VERSION 3.3)

add_library(shell
	inc/shell/detail/ConfigConsoleHistory.h
	inc/shell/Config.h
	inc/shell/Desktop.h
	inc/shell/Profiler.h

	BotView.h
	Campaign.h
	CampaignControlCommands.h
	CampaignControls.h
	ConfigWidgets.h
	Game.h
	GamePauseMenu.h
	GetFileName.h
	gui.h	
	InputManager.h
	KeyMapper.h
	LuaConsole.h
	MainMenu.h
	MapList.h
	MapPreview.h
	MessageArea.h
	NavStack.h
	Network.h
	PlayerView.h
	ScoreTable.h
	SelectMapDlg.h
	Settings.h
	SinglePlayer.h
	VehicleStateReader.h
	Widgets.h

	BotView.cpp
	Campaign.cpp
	CampaignControls.cpp
	Config.cpp
	ConfigConsoleHistory.cpp
	ConfigWidgets.cpp
	Desktop.cpp
	Game.cpp
	GamePauseMenu.cpp
	GetFileName.cpp
	gui.cpp
	InputManager.cpp
	KeyMapper.cpp
	LuaConsole.cpp
	MainMenu.cpp
	MapList.cpp
	MapPreview.cpp
	MessageArea.cpp
	NavStack.cpp
#	Network.cpp
	PlayerView.cpp
	Profiler.cpp
	ScoreTable.cpp
	SelectMapDlg.cpp
	Settings.cpp
	SinglePlayer.cpp
	VehicleStateReader.cpp
	Widgets.cpp
)

target_link_libraries(shell PRIVATE
	as
	cbind
	config
	ctx
	editor
	fs
	gc
	gclua
	gv
	loc
	mapfile
	plat
	ui
	lua

	PUBLIC editor render
)

target_include_directories(shell INTERFACE inc)
set_target_properties(shell PROPERTIES FOLDER game)
