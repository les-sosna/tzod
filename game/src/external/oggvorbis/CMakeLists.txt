cmake_minimum_required (VERSION 3.3)
if(WIN32)
    add_definitions(-D_CRT_SECURE_NO_WARNINGS)
endif()
add_library(oggvorbis
	ogg/bitwise.c
	ogg/framing.c
	vorbis/analysis.c
	vorbis/bitrate.c
	vorbis/block.c
	vorbis/codebook.c
	vorbis/envelope.c
	vorbis/floor0.c
	vorbis/floor1.c
	vorbis/info.c
#	vorbis/lookup.c
	vorbis/lpc.c
	vorbis/lsp.c
	vorbis/mapping0.c
	vorbis/mdct.c
	vorbis/psy.c
	vorbis/registry.c
	vorbis/res0.c
	vorbis/sharedbook.c
	vorbis/smallft.c
	vorbis/synthesis.c
	vorbis/vorbisfile.c
	vorbis/window.c
)

target_include_directories(oggvorbis PUBLIC include)
if (WIN32)
	target_compile_options(oggvorbis PRIVATE
		/wd4244 # conversion from 'double' to 'long', possible loss of data
		/wd4305 # truncation from 'double' to 'float'
	)
endif()
