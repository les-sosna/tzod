cmake_minimum_required (VERSION 3.3)
project(Audio)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${AS_SOURCE_DIR}/inc
	${Ctx_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${GC_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc

	${OggVorbis_SOURCE_DIR}/include
)

set(Audio_SOURCES
	OggVorbis.cpp
	SoundHarness.cpp
	SoundView.cpp
)

set(Audio_LIBS
	filesystem
	gc
	oggvorbis
)

if(WIN32)
	list(APPEND Audio_SOURCES SoundRenderXA2.cpp)
	list(APPEND Audio_LIBS Xaudio2)
else()
	find_package(OpenAL)
	include_directories(${OPENAL_INCLUDE_DIR})
	list(APPEND Audio_SOURCES SoundRenderOAL.cpp AudioContextOAL.cpp MusicPlayer.cpp)
	list(APPEND Audio_LIBS ${OPENAL_LIBRARY})
endif()

add_library(audio ${Audio_SOURCES})
target_link_libraries(audio ${Audio_LIBS})
