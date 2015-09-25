cmake_minimum_required (VERSION 3.3)
project(Audio)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

find_package(OpenAL)

include_directories(
	${App_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${GC_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc

	${OggVorbis_SOURCE_DIR}/include
	${OPENAL_INCLUDE_DIR}
)

add_library(audio
	AudioContext.cpp
	MusicPlayer.cpp
	SoundHarness.cpp
	SoundRender.cpp
	SoundView.cpp
)

target_link_libraries(audio
	filesystem
	gc
	oggvorbis
	${OPENAL_LIBRARY}
)
