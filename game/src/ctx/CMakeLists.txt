cmake_minimum_required (VERSION 3.3)
project(Ctx)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${AI_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${GC_SOURCE_DIR}/inc
	${MapFile_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
	${Script_SOURCE_DIR}/inc
)

add_library(ctx
	AIManager.cpp
	Deathmatch.cpp
	EditorContext.cpp
	GameContext.cpp
	GameEvents.cpp
	ScriptMessageBroadcaster.cpp
	WorldController.cpp
)

target_link_libraries(ctx
	ai
	filesystem
	gc
	script
)
