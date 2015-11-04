cmake_minimum_required (VERSION 3.3)
project(AS)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${Ctx_SOURCE_DIR}/inc
#	${AI_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
#	${GC_SOURCE_DIR}/inc
#	${MapFile_SOURCE_DIR}/inc
#	${Math_SOURCE_DIR}/inc
	${Script_SOURCE_DIR}/inc
)

add_library(as
	AppController.cpp
	AppState.cpp
	AppStateListener.cpp
)

target_link_libraries(as
	ctx
	filesystem
)
