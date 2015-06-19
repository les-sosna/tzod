cmake_minimum_required (VERSION 2.8)
project(Tank)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.55 REQUIRED COMPONENTS system)

find_package(OpenAL)

set(tank_INCLUDE_DIRS
	.
	${AI_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
	${GC_SOURCE_DIR}/inc
	${Render_SOURCE_DIR}/inc
	${UI_SOURCE_DIR}/inc
	${MapFile_SOURCE_DIR}/inc
	${Video_SOURCE_DIR}/inc
	
	# 3rd party
	${Boost_INCLUDE_DIRS}
	${Lua_SOURCE_DIR}/src
	${Zlib_SOURCE_DIR}
	${OggVorbis_SOURCE_DIR}/include
	${Pluto_SOURCE_DIR}
	${GLFW_SOURCE_DIR}/include
)

set(tank_SOURCES
	AIManager.cpp
	AppState.cpp
	Controller.cpp
	Deathmatch.cpp
	DefaultCamera.cpp
	EditorContext.cpp
	GameContext.cpp
	GameEvents.cpp
	GameView.cpp
	GlfwPlatform.cpp
	globals.cpp
	gui.cpp
	gui_campaign.cpp
	gui_desktop.cpp
	gui_editor.cpp
	gui_game.cpp
	gui_getfilename.cpp
	gui_mainmenu.cpp
	gui_maplist.cpp
	gui_messagearea.cpp
	gui_network.cpp
	gui_scoretable.cpp
	gui_settings.cpp
	gui_widgets.cpp
	InputManager.cpp
	KeyMapper.cpp
	Md5.c
	ThemeManager.cpp
	WorldController.cpp
	config/Config.cpp
	config/ConfigBase.cpp
	config/Language.cpp
	core/Profiler.cpp
	core/Timer.cpp
	gclua/lgcmod.cpp
#	gclua/lGame.cpp
	gclua/lObject.cpp
	gclua/lObjUtil.cpp
	gclua/lWorld.cpp
#	network/CommonTypes.cpp
#	network/ControlPacket.cpp
#	network/HttpClient.cpp
#	network/init.cpp
#	network/LobbyClient.cpp
#	network/Peer.cpp
#	network/Socket.cpp
#	network/TankClient.cpp
#	network/TankServer.cpp
#	network/Variant.cpp
	script/script.cpp
	script/ScriptHarness.cpp
	script/sTrigger.cpp
	script/sRigidBodyStatic.cpp
	script/sPickup.cpp
	script/sPlayer.cpp
)

set(tank_LIBRARIES
	ai
	filesystem
	gc
	render
	ui
	lua
	pluto
	zlib
	glfw
	oggvorbis
	${GLFW_LIBRARIES}
	${Boost_LIBRARIES}
)

if(${OPENAL_FOUND})
	list(APPEND tank_LIBRARIES ${OPENAL_LIBRARY})
	list(APPEND tank_INCLUDE_DIRS ${OPENAL_INCLUDE_DIR})
	list(APPEND tank_SOURCES
		AudioContext.cpp
		SoundHarness.cpp
		SoundRender.cpp
		SoundView.cpp
		sound/MusicPlayer.cpp
	)
else()
	add_definitions(-DNOSOUND)
endif()

include_directories(${tank_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIR})
add_library(tank ${tank_SOURCES})
target_link_libraries(tank ${tank_LIBRARIES})
