cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

add_library(loc Language.cpp)

target_link_libraries(loc config)

target_include_directories(loc INTERFACE inc)
