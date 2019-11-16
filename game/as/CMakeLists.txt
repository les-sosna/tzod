cmake_minimum_required (VERSION 3.3)

add_library(as
	inc/as/AppConstants.h
	inc/as/AppController.h
	inc/as/AppState.h
	inc/as/AppStateListener.h
	inc/as/MapCollection.h

	AppController.cpp
	AppState.cpp
	AppStateListener.cpp
	MapCollection.cpp
)

target_link_libraries(as PRIVATE
	ctx
	gc
	fs
	mapfile
)

target_include_directories(as INTERFACE inc)
set_target_properties(as PROPERTIES FOLDER game)