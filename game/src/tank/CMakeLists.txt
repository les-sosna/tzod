cmake_minimum_required (VERSION 2.8)
project(Tank)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.55 REQUIRED COMPONENTS system)

#find_package(OpenAL)

include_directories(
	${Boost_INCLUDE_DIRS}
	${Lua_SOURCE_DIR}/src
	${Zlib_SOURCE_DIR}
	${OggVorbis_SOURCE_DIR}/include
	${Pluto_SOURCE_DIR}
	${GLFW_SOURCE_DIR}/include
	.
)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
add_definitions(-DNOSOUND)
add_definitions(-DNOMINMAX) # win32 specific
#link_libraries(${glfw_LIBRARIES})
#link_directories(${Boost_LIBRARY_DIR})
set(tank_SOURCES
	BackgroundIntro.cpp
	ClientBase.cpp
	Controller.cpp
	DefaultCamera.cpp
#	directx.cpp
#	Dsutil.cpp
	functions.cpp
	globals.cpp
#	InputManager.cpp
	KeyMapper.cpp
	Level.cpp
	Main.cpp
	Md5.c
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
	fs/FileSystemImpl.cpp
	fs/MapFile.cpp
	fs/SaveFile.cpp
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
	ui/Button.cpp
	ui/Combo.cpp
	ui/Console.cpp
	ui/ConsoleBuffer.cpp
	ui/Dialog.cpp
	ui/Edit.cpp
	ui/gui.cpp
	ui/gui_campaign.cpp
	ui/gui_desktop.cpp
	ui/gui_editor.cpp
	ui/gui_getfilename.cpp
	ui/gui_mainmenu.cpp
	ui/gui_maplist.cpp
	ui/gui_network.cpp
	ui/gui_scoretable.cpp
	ui/gui_settings.cpp
	ui/gui_widgets.cpp
	ui/GuiManager.cpp
#	ui/Interface.cpp
	ui/List.cpp
	ui/ListBase.cpp
	ui/MousePointer.cpp
	ui/Scroll.cpp
	ui/Text.cpp
	ui/Window.cpp
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

if(WIN32)
	add_executable(tank WIN32 ${tank_SOURCES})
elseif(APPLE)
	add_executable(tank MACOSX_BUNDLE ${tank_SOURCES})
else()
	message(FATAL_ERROR "Unknown platform")
endif()

target_link_libraries(tank lua pluto zlib glfw ${GLFW_LIBRARIES} ${Boost_LIBRARIES})
