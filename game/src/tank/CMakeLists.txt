cmake_minimum_required (VERSION 2.8)
project(Tank)

add_subdirectory(fs)
add_subdirectory(ui)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.55 REQUIRED COMPONENTS system)

#find_package(OpenAL)

include_directories(
	.
	${FileSystem_SOURCE_DIR}
	${UI_SOURCE_DIR}
	
	# 3rd party
	${Boost_INCLUDE_DIRS}
	${Lua_SOURCE_DIR}/src
	${Zlib_SOURCE_DIR}
	${OggVorbis_SOURCE_DIR}/include
	${Pluto_SOURCE_DIR}
	${GLFW_SOURCE_DIR}/include
)

set(tank_SOURCES
	BackgroundIntro.cpp
	ClientBase.cpp
	Controller.cpp
	DefaultCamera.cpp
#	directx.cpp
#	Dsutil.cpp
	globals.cpp
	gui.cpp
	gui_campaign.cpp
	gui_desktop.cpp
	gui_editor.cpp
	gui_getfilename.cpp
	gui_mainmenu.cpp
	gui_maplist.cpp
	gui_network.cpp
	gui_scoretable.cpp
	gui_settings.cpp
	gui_widgets.cpp
#	InputManager.cpp
	KeyMapper.cpp
	Level.cpp
	Main.cpp
	MapFile.cpp
	Md5.c
	MousePointer.cpp
	Pause.cpp
	SaveFile.cpp
	script.cpp
	SinglePlayer.cpp
	config/Config.cpp
	config/ConfigBase.cpp
	config/Language.cpp
	core/Profiler.cpp
	core/Rotator.cpp
	core/SafePtr.cpp
	core/Timer.cpp
	video/ImageLoader.cpp
#	video/RenderDirect3D.cpp
	video/RenderOpenGL.cpp
	video/TextureManager.cpp
	gc/TypeSystem.cpp
	gc/2dSprite.cpp
	gc/Actor.cpp
	gc/ai.cpp
	gc/Camera.cpp
	gc/crate.cpp
	gc/GameClasses.cpp
	gc/indicators.cpp
	gc/Light.cpp
	gc/MessageBox.cpp
	gc/Object.cpp
	gc/particles.cpp
	gc/pickup.cpp
	gc/Player.cpp
	gc/projectiles.cpp
	gc/RigidBody.cpp
	gc/RigidBodyDynamic.cpp
	gc/Service.cpp
	gc/Sound.cpp
	gc/Trigger.cpp
	gc/Turrets.cpp
	gc/UserObjects.cpp
	gc/Vehicle.cpp
	gc/Weapons.cpp
#	network/CommonTypes.cpp
	network/ControlPacket.cpp
#	network/HttpClient.cpp
#	network/init.cpp
#	network/LobbyClient.cpp
#	network/Peer.cpp
#	network/Socket.cpp
#	network/TankClient.cpp
#	network/TankServer.cpp
#	network/Variant.cpp
#	sound/MusicPlayer.cpp
#	sound/sfx.cpp
)

add_definitions(-DNOSOUND)
link_directories(${Boost_LIBRARY_DIR})

if(WIN32)
	add_definitions(-DNOMINMAX)
	add_executable(tank WIN32 ${tank_SOURCES})
elseif(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
	add_executable(tank MACOSX_BUNDLE ${tank_SOURCES})
else()
	message(FATAL_ERROR "Unknown platform")
endif()

target_link_libraries(tank 
	libFileSystem
	libUI
	lua
	pluto
	zlib
	glfw
	${GLFW_LIBRARIES}
	${Boost_LIBRARIES}
)
