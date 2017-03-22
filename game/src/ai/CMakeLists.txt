cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(ai
	inc/ai/ai.h

	ai.cpp
	DrivingAgent.cpp
	DrivingAgent.h
	ShootingAgent.cpp
	ShootingAgent.h
)

target_link_libraries(ai
	gc
	filesystem
)

target_include_directories(ai INTERFACE inc)
