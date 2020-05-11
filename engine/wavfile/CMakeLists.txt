add_library(wavfile
	inc/wavfile/WavFile.h
	WavFile.cpp
)

target_link_libraries(wavfile PRIVATE fs)

target_include_directories(wavfile INTERFACE inc)
set_target_properties(wavfile PROPERTIES FOLDER engine)