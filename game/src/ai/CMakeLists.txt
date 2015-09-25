cmake_minimum_required (VERSION 3.3)
project(AI)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${FileSystem_SOURCE_DIR}/inc
	${GC_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
)

add_library(ai
	ai.cpp
)

target_link_libraries(ai
	gc
	filesystem
)
