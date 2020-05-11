cmake_minimum_required (VERSION 3.6)
project(tzod VERSION 1.52)

set_property(GLOBAL PROPERTY USE_FOLDERS ON)

if(WIN32 OR APPLE OR UNIX)
	set(CMAKE_CXX_STANDARD 17)
endif()

add_subdirectory(external)
add_subdirectory(engine)
add_subdirectory(game)
