cmake_minimum_required (VERSION 3.3)
project(AS)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${Config_SOURCE_DIR}/inc
	${Ctx_SOURCE_DIR}/inc
#	${AI_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
#	${GC_SOURCE_DIR}/inc
#	${MapFile_SOURCE_DIR}/inc
#	${Math_SOURCE_DIR}/inc
	${Script_SOURCE_DIR}/inc
)

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
