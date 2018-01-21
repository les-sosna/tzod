cmake_minimum_required (VERSION 3.4)
project(tzod VERSION 1.52)

if(APPLE OR UNIX)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14")
endif()

if(WIN32)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std:c++latest")
	set(WITH_SOUND 1)
else()
	find_package(OpenAL)
	set(WITH_SOUND ${OPENAL_FOUND})
endif()

# external libs
add_subdirectory(external)

# libs
add_subdirectory(luaetc)
add_subdirectory(fs)
add_subdirectory(config)
add_subdirectory(loc)
add_subdirectory(math)
add_subdirectory(plat)
add_subdirectory(platetc)
add_subdirectory(mf)
add_subdirectory(gc)
add_subdirectory(ai)
add_subdirectory(gclua)
add_subdirectory(script)
add_subdirectory(ctx)
add_subdirectory(as)
add_subdirectory(video)
add_subdirectory(render)
add_subdirectory(gv)
add_subdirectory(libui)
add_subdirectory(cbind)
add_subdirectory(editor)
add_subdirectory(shell)
if(WITH_SOUND)
	add_subdirectory(audio)
endif()

# top level lib - the only one the application links with
add_subdirectory(app)

# applications
if((NOT IOS) AND (NOT WINRT))
	add_subdirectory(platglfw)
	add_subdirectory(tzodmain)
	add_subdirectory(gc_tests)
	add_subdirectory(uitestapp)
endif()

