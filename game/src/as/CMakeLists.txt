cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(as
	inc/as/AppConfig.h
	inc/as/AppConstants.h
	inc/as/AppController.h
	inc/as/AppState.h
	inc/as/AppStateListener.h

	AppConfig.cpp
	AppController.cpp
	AppState.cpp
	AppStateListener.cpp
)

target_link_libraries(as
	config
	ctx
	filesystem
)

target_include_directories(as INTERFACE inc)
