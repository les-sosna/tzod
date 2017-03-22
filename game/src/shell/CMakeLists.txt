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
	ConfigBinding.h
	Controller.h
	Editor.h
	Game.h
	GameClassVis.h
	GetFileName.h
	gui.h	
	InputManager.h
	KeyMapper.h
	MainMenu.h
	MapList.h
	MapPreview.h
	MapSettings.h
	MessageArea.h
	NavStack.h
	Network.h
	PlayerView.h
	PropertyList.h
	QuickActions.h
	ScoreTable.h
	ServiceEditor.h
	Settings.h
	SinglePlayer.h
	Widgets.h

	BotView.cpp
	Campaign.cpp
	CampaignControls.cpp
	Config.cpp
	ConfigBinding.cpp
	ConfigConsoleHistory.cpp
	Controller.cpp
	DefaultCamera.cpp
	Desktop.cpp
	Editor.cpp
	Game.cpp
	GameClassVis.cpp
	GetFileName.cpp
	gui.cpp
	InputManager.cpp
	KeyMapper.cpp
	MainMenu.cpp
	MapList.cpp
	MapPreview.cpp
	MapSettings.cpp
	MessageArea.cpp
	NavStack.cpp
#	Network.cpp
	SinglePlayer.cpp
	PlayerView.cpp
	Profiler.cpp
	PropertyList.cpp
	QuickActions.cpp
	ScoreTable.cpp
#	ServiceEditor.cpp
	Settings.cpp
	Widgets.cpp
)

target_link_libraries(shell
	as
	config
	loc
	gc
	gclua
	gv
	mapfile
	render
	ui
	lua
	luaetc
)

target_include_directories(shell INTERFACE inc)
