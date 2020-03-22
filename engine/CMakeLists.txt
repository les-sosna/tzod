cmake_minimum_required (VERSION 3.6)

if(APPLE OR UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
endif()

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std:c++latest")
	add_subdirectory(fswin)
else()
	add_subdirectory(fsposix)
endif()

# libs
add_subdirectory(luaetc)
add_subdirectory(fs)
add_subdirectory(config)
add_subdirectory(math)
add_subdirectory(plat)
add_subdirectory(platetc)
add_subdirectory(video)
add_subdirectory(wavfile)
add_subdirectory(ui)
add_subdirectory(cbind)

# desktop applications
if((NOT IOS) AND (NOT WINRT) AND (NOT ANDROID))
	message("Building for desktop with GLFW")

	add_subdirectory(platglfw)
	add_subdirectory(fsmem)
	add_subdirectory(ui_tests)
	add_subdirectory(video_tests)
	add_subdirectory(uitestapp)
endif()

if(ANDROID)
	message("Building for Android with JNI")

	add_subdirectory(fsjni)
	add_subdirectory(platjni)
endif()
