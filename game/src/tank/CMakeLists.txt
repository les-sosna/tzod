cmake_minimum_required (VERSION 2.8)
project(Tank)

set(Boost_USE_STATIC_LIBS        ON)
set(Boost_USE_MULTITHREADED      ON)
set(Boost_USE_STATIC_RUNTIME    OFF)
find_package(Boost 1.55 REQUIRED COMPONENTS system)

find_package(OpenAL)

set(tank_INCLUDE_DIRS
	.
	${FileSystem_SOURCE_DIR}/include
	${Math_SOURCE_DIR}/include
	${UI_SOURCE_DIR}/include
	${Video_SOURCE_DIR}/include
	
	# 3rd party
	${Boost_INCLUDE_DIRS}
	${Lua_SOURCE_DIR}/src
	${Zlib_SOURCE_DIR}
	${OggVorbis_SOURCE_DIR}/include
	${Pluto_SOURCE_DIR}
	${GLFW_SOURCE_DIR}/include
)

set(tank_SOURCES
	ai.cpp
	AIManager.cpp
	BackgroundIntro.cpp
	ClientBase.cpp
	Controller.cpp
	Deathmatch.cpp
	DefaultCamera.cpp
	GameContext.cpp
	GameEvents.cpp
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
	MapFile.cpp
	Md5.c
	SaveFile.cpp
	ThemeManager.cpp
	WorldController.cpp
	config/Config.cpp
	config/ConfigBase.cpp
	config/Language.cpp
	core/Profiler.cpp
	core/Timer.cpp
	gc/Actor.cpp
	gc/Camera.cpp
	gc/Crate.cpp
	gc/Explosion.cpp
	gc/Field.cpp
	gc/GameClasses.cpp
	gc/Indicators.cpp
	gc/Light.cpp
#	gc/MessageBox.cpp
	gc/Object.cpp
	gc/ObjectProperty.cpp
	gc/particles.cpp
	gc/Pickup.cpp
	gc/Player.cpp
	gc/Projectiles.cpp
	gc/RigidBody.cpp
	gc/RigidBodyDynamic.cpp
	gc/Rotator.cpp
	gc/Service.cpp
	gc/Trigger.cpp
	gc/Turrets.cpp
	gc/TypeSystem.cpp
	gc/UserObjects.cpp
	gc/Vehicle.cpp
	gc/VehicleClasses.cpp
	gc/WeaponBase.cpp
	gc/Weapons.cpp
	gc/World.cpp
	gclua/lgcmod.cpp
	gclua/lGame.cpp
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
	render/WorldView.cpp
	render/ObjectViewsSelector.cpp
	render/RenderScheme.cpp
	render/rAnimatedSprite.cpp
	render/rBrickFragment.cpp
	render/rBooster.cpp
	render/rDecoration.cpp
	render/rFireSpark.cpp
	render/rIndicator.cpp
	render/rLight.cpp
	render/rMinigun.cpp
	render/rParticle.cpp
	render/rShock.cpp
	render/rSprite.cpp
	render/rText.cpp
	render/rTile.cpp
	render/rTurret.cpp
	render/rUserObject.cpp
	render/rVehicle.cpp
	render/rWall.cpp
	render/rWeapon.cpp
	render/rWeaponBase.cpp
	render/Terrain.cpp
	script/script.cpp
	script/ScriptHarness.cpp
	script/sTrigger.cpp
	script/sRigidBodyStatic.cpp
	script/sPickup.cpp
	script/sPlayer.cpp
)

set(tank_LIBRARIES
	filesystem
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
		sound/MusicPlayer.cpp
	)
else()
	add_definitions(-DNOSOUND)
endif()

include_directories(${tank_INCLUDE_DIRS})
link_directories(${Boost_LIBRARY_DIR})
add_library(tank ${tank_SOURCES})
target_link_libraries(tank ${tank_LIBRARIES})
