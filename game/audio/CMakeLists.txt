cmake_minimum_required (VERSION 3.3)

set(Audio_SOURCES
	inc/audio/SoundRender.h
	inc/audio/SoundView.h

#	OggVorbis.cpp
#	OggVorbis.h
	SoundHarness.cpp
	SoundHarness.h
	SoundTemplates.h
	SoundView.cpp
)

set(Audio_LIBS
	as
	ctx
	fs
	gc
	plat
	wavfile

	# external
#	oggvorbis
)

set(Audio_INCLUDE_DIRS)

if(WIN32)
	list(APPEND Audio_SOURCES SoundRenderXA2.cpp)
	list(APPEND Audio_LIBS Xaudio2)
else()
	find_package(OpenAL)
	list(APPEND Audio_INCLUDE_DIRS ${OPENAL_INCLUDE_DIR})
	list(APPEND Audio_SOURCES SoundRenderOAL.cpp AudioContextOAL.cpp)# MusicPlayer.cpp)
	list(APPEND Audio_LIBS ${OPENAL_LIBRARY})
endif()

add_library(audio ${Audio_SOURCES})

target_link_libraries(audio PRIVATE ${Audio_LIBS})

target_include_directories(audio INTERFACE inc)
target_include_directories(audio PRIVATE ${Audio_INCLUDE_DIRS})
set_target_properties(audio PROPERTIES FOLDER game)