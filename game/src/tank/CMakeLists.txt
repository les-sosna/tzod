cmake_minimum_required (VERSION 3.3)
project(Tank)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.59 REQUIRED COMPONENTS system date_time regex)

find_package(OpenAL)

set(tank_INCLUDE_DIRS
	${AI_SOURCE_DIR}/inc
	${App_SOURCE_DIR}/inc
	${Config_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${GC_SOURCE_DIR}/inc
	${GCLua_SOURCE_DIR}/inc
	${Loc_SOURCE_DIR}/inc
	${MapFile_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
	${Render_SOURCE_DIR}/inc
	${Script_SOURCE_DIR}/inc
	${UI_SOURCE_DIR}/inc
	${Video_SOURCE_DIR}/inc

	# 3rd party
	${Boost_INCLUDE_DIRS}
	${GLFW_SOURCE_DIR}/include
	${Lua_SOURCE_DIR}/src
	${Pluto_SOURCE_DIR}
	${Zlib_SOURCE_DIR}
)

set(tank_SOURCES
	Controller.cpp
	ConfigConsoleHistory.cpp
	DefaultCamera.cpp
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
	Config.cpp
	core/Profiler.cpp
	core/Timer.cpp
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
)

set(tank_LIBRARIES
	ai
	app
	config
	loc
	filesystem
	gc
	gclua
	render
	ui
	lua
	pluto
	zlib
	glfw
	${GLFW_LIBRARIES}
	${Boost_LIBRARIES}
)

if(${OPENAL_FOUND})
	list(APPEND tank_LIBRARIES audio)
	list(APPEND tank_INCLUDE_DIRS ${Audio_SOURCE_DIR}/inc)
else()
	add_definitions(-DNOSOUND)
endif()

include_directories(${tank_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIR})
add_library(tank ${tank_SOURCES})
target_link_libraries(tank ${tank_LIBRARIES})
