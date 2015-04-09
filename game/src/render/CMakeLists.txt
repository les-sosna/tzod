cmake_minimum_required (VERSION 2.8)
project(Render)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${GC_SOURCE_DIR}/include
	${Math_SOURCE_DIR}/include
	${Video_SOURCE_DIR}/include
)

add_library(render
	WorldView.cpp
	ObjectViewsSelector.cpp
	RenderScheme.cpp
	rAnimatedSprite.cpp
	rBrickFragment.cpp
	rBooster.cpp
	rDecoration.cpp
	rFireSpark.cpp
	rIndicator.cpp
	rLight.cpp
	rMinigun.cpp
	rParticle.cpp
	rShock.cpp
	rSprite.cpp
	rText.cpp
	rTile.cpp
	rTurret.cpp
	rUserObject.cpp
	rVehicle.cpp
	rWall.cpp
	rWeapon.cpp
	rWeaponBase.cpp
	Terrain.cpp
)

target_link_libraries(render
	gc
	video
)

