cmake_minimum_required (VERSION 3.3)

add_library(ai
	inc/ai/ai.h

	ai.cpp
	DrivingAgent.cpp
	DrivingAgent.h
	ShootingAgent.cpp
	ShootingAgent.h
)

target_link_libraries(ai
	PRIVATE gc
	PRIVATE fs
)

target_include_directories(ai INTERFACE inc)
