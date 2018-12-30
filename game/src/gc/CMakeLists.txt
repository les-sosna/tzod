cmake_minimum_required (VERSION 3.3)

add_library(gc
	inc/gc/Actor.h
	inc/gc/Crate.h
	inc/gc/Explosion.h
	inc/gc/Field.h
	inc/gc/GameClasses.h
	inc/gc/Grid.h
	inc/gc/Light.h
	inc/gc/Macros.h
	inc/gc/MessageBox.h
	inc/gc/NeighborAware.h
	inc/gc/Object.h
	inc/gc/ObjectProperty.h
	inc/gc/ObjPtr.h
	inc/gc/Particles.h
	inc/gc/Pickup.h
	inc/gc/Player.h
	inc/gc/Projectiles.h
	inc/gc/RigidBody.h
	inc/gc/RigidBodyDynamic.h
	inc/gc/SaveFile.h
	inc/gc/Serialization.h
	inc/gc/Service.h
	inc/gc/SpawnPoint.h
	inc/gc/Trigger.h
	inc/gc/Turrets.h
	inc/gc/TypeSystem.h
	inc/gc/UserObjects.h
	inc/gc/Vehicle.h
	inc/gc/VehicleClasses.h
	inc/gc/VehicleState.h
	inc/gc/Wall.h
	inc/gc/Water.h
	inc/gc/WeapCfg.h
	inc/gc/WeaponBase.h
	inc/gc/Weapons.h
	inc/gc/World.h
	inc/gc/World.inl
	inc/gc/WorldCache.h
	inc/gc/WorldCfg.h
	inc/gc/WorldEvents.h
	inc/gc/Z.h	

	inc/gc/detail/GlobalListHelper.h
	inc/gc/detail/JobManager.h
	inc/gc/detail/MemoryManager.h
	inc/gc/detail/PtrList.h
	inc/gc/detail/Rotator.h

	Actor.cpp
	Crate.cpp
	Explosion.cpp
	Field.cpp
	GameClasses.cpp
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
	SpawnPoint.cpp
	Trigger.cpp
	Turrets.cpp
	TypeReg.h
	TypeSystem.cpp
	UserObjects.cpp
	Vehicle.cpp
	VehicleClasses.cpp
	Wall.cpp
	Water.cpp
	WeaponBase.cpp
	Weapons.cpp
	World.cpp
	WorldCache.cpp
)

target_link_libraries(gc
	PRIVATE mapfile
	PUBLIC fs math
)

target_include_directories(gc INTERFACE inc)
