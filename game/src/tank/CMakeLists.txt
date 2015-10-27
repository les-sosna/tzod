cmake_minimum_required (VERSION 3.3)
project(Tank)

find_package(OpenAL)

set(tank_INCLUDE_DIRS
	${LibUI_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
	${Video_SOURCE_DIR}/inc

	# 3rd party
	${GLFW_SOURCE_DIR}/include
)

set(tank_SOURCES
	GlfwPlatform.cpp
	Md5.c
	core/Timer.cpp
#	network/CommonTypes.cpp
#	network/ControlPacket.cpp
#	network/HttpClient.cpp
#	network/init.cpp
#	network/LobbyClient.cpp
#	network/Peer.cpp
#	network/Socket.cpp
#	network/TankClient.cpp
#	network/TankServer.cpp
#	network/Variant.cpp
)

set(tank_LIBRARIES
	ui
	glfw
	${GLFW_LIBRARIES}
)

if(${OPENAL_FOUND})
	list(APPEND tank_LIBRARIES audio)
	list(APPEND tank_INCLUDE_DIRS ${Audio_SOURCE_DIR}/inc)
else()
	add_definitions(-DNOSOUND)
endif()

include_directories(${tank_INCLUDE_DIRS})
add_library(tank ${tank_SOURCES})
target_link_libraries(tank ${tank_LIBRARIES})
