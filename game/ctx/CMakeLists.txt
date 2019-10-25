cmake_minimum_required (VERSION 3.3)

add_library(ctx
	inc/ctx/AIManager.h
	inc/ctx/AppConfig.h
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
	AppConfig.cpp
	Deathmatch.cpp
	EditorContext.cpp
	GameContext.cpp
	GameEvents.cpp
	ScriptMessageBroadcaster.cpp
	WorldController.cpp
)

target_link_libraries(ctx PRIVATE
	ai
	fs
	gc
	mapfile
	PUBLIC config script
)

target_include_directories(ctx INTERFACE inc)
set_target_properties(ctx PROPERTIES FOLDER game)