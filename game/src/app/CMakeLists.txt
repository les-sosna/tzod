cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(app
	inc/app/tzod.h
	inc/app/View.h

	CombinedConfig.cpp
	CombinedConfig.h
	tzod.cpp
	View.cpp
)

if(WITH_SOUND)
	target_link_libraries(app audio)
else()
	add_definitions(-DNOSOUND)
endif()

target_link_libraries(app
	as
	ctx
	loc
	shell
	video
)

target_include_directories(app INTERFACE inc)
