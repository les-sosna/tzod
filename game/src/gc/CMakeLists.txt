cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(gc
	Actor.cpp
	Crate.cpp
	Explosion.cpp
	Field.cpp
	GameClasses.cpp
	Indicators.cpp
	Light.cpp
#	MessageBox.cpp
	MapCache.cpp
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
	Wall.cpp
	Water.cpp
	WeaponBase.cpp
	Weapons.cpp
	World.cpp
)

target_link_libraries(gc
	PRIVATE filesystem
	PRIVATE mapfile
	PUBLIC math
)

target_include_directories(gc INTERFACE inc)
