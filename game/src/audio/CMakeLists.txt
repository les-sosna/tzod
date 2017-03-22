cmake_minimum_required (VERSION 3.3)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

set(Audio_SOURCES
	OggVorbis.cpp
	SoundHarness.cpp
	SoundView.cpp
)

set(Audio_LIBS
	filesystem
	gc
	ui #fixme: logger

	# external
	oggvorbis
)

if(WIN32)
	list(APPEND Audio_SOURCES SoundRenderXA2.cpp)
	list(APPEND Audio_LIBS Xaudio2)
else()
	find_package(OpenAL)
	target_include_directories(audio PRIVATE ${OPENAL_INCLUDE_DIR})
	list(APPEND Audio_SOURCES SoundRenderOAL.cpp AudioContextOAL.cpp MusicPlayer.cpp)
	list(APPEND Audio_LIBS ${OPENAL_LIBRARY})
endif()

add_library(audio ${Audio_SOURCES})

target_link_libraries(audio PRIVATE
	as
	ctx
	${Audio_LIBS}
)

target_include_directories(audio INTERFACE inc)
