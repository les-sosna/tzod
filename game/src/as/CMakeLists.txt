cmake_minimum_required (VERSION 3.3)

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

target_link_libraries(as PRIVATE
	config
	ctx
	gc
	fs
)

target_include_directories(as INTERFACE inc)
