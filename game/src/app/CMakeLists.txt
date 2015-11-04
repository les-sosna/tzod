cmake_minimum_required (VERSION 3.3)
project(App)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
)

add_library(app
	tzod.cpp
)

target_link_libraries(app
)
