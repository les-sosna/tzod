cmake_minimum_required (VERSION 2.8)
project(GC)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	include
	${FileSystem_SOURCE_DIR}/include
	${MapFile_SOURCE_DIR}/include
	${Math_SOURCE_DIR}/include
)

add_library(gc
	Actor.cpp
	Camera.cpp
	Crate.cpp
	Explosion.cpp
	Field.cpp
	GameClasses.cpp
	Indicators.cpp
	Light.cpp
#	MessageBox.cpp
	Object.cpp
	ObjectProperty.cpp
	Particles.cpp
	Pickup.cpp
	Player.cpp
	Projectiles.cpp
	RigidBody.cpp
	RigidBodyDynamic.cpp
	Rotator.cpp
	SaveFile.cpp
	Service.cpp
	Trigger.cpp
	Turrets.cpp
	TypeSystem.cpp
	UserObjects.cpp
	Vehicle.cpp
	VehicleClasses.cpp
	WeaponBase.cpp
	Weapons.cpp
	World.cpp
)

target_link_libraries(gc
	filesystem
	mapfile
)

