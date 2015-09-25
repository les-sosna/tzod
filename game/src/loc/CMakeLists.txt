cmake_minimum_required (VERSION 3.3)
project(Loc)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${Config_SOURCE_DIR}/inc
)

add_library(loc
	Language.cpp
)

target_link_libraries(loc
	config
)
