cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

add_library(ctx
	inc/ctx/Deathmatch.h
	inc/ctx/EditorContext.h
	inc/ctx/GameContext.h
	inc/ctx/GameContextBase.h
	inc/ctx/GameEvents.h
	inc/ctx/Gameplay.h
	inc/ctx/ScriptMessageBroadcaster.h
	inc/ctx/ScriptMessageSource.h
	inc/ctx/WorldController.h

	AIManager.cpp
	Deathmatch.cpp
	EditorContext.cpp
	GameContext.cpp
	GameEvents.cpp
	ScriptMessageBroadcaster.cpp
	WorldController.cpp
)

target_link_libraries(ctx PRIVATE
	ai
	filesystem
	gc
	mapfile
	PUBLIC script
)

target_include_directories(ctx INTERFACE inc)
