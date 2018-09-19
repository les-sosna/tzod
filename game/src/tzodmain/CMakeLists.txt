cmake_minimum_required (VERSION 3.3)

link_directories(${Boost_LIBRARY_DIR})

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x0501) # Windows XP
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DNOMINMAX)
	add_executable(tzodmain WIN32 Main.cpp)
	set_target_properties(tzodmain PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/..")
	set(FSLIB fswin)
elseif(APPLE)
	add_executable(tzodmain Main.cpp) # hack to enable retina display support
#	add_executable(tzodmain MACOSX_BUNDLE Main.cpp)
	set(FSLIB fsstdio)
elseif(UNIX)
	add_executable(tzodmain Main.cpp)
	set(FSLIB fsstdio)
else()
	message(FATAL_ERROR "Unknown platform")
endif()

target_link_libraries(tzodmain PRIVATE
	app
	as
	platglfw
	${FSLIB}
)
