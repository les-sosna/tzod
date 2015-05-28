cmake_minimum_required (VERSION 2.8)
project(AI)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${FileSystem_SOURCE_DIR}/include
	${GC_SOURCE_DIR}/include
	${Math_SOURCE_DIR}/include
)

add_library(ai
	ai.cpp
)

target_link_libraries(ai
	gc
	filesystem
)
