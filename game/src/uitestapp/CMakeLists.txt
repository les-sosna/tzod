cmake_minimum_required (VERSION 3.3)

link_directories(${Boost_LIBRARY_DIR})

set(UITestApp_SOURCES
	UITestDesktop.cpp
	UITestMain.cpp
)

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x0501) # Windows XP
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DNOMINMAX)
	add_executable(uitestapp WIN32 ${UITestApp_SOURCES})
	set_target_properties(uitestapp PROPERTIES VS_DEBUGGER_WORKING_DIRECTORY "${CMAKE_SOURCE_DIR}/..")
	set(FSLIB fswin)
elseif(APPLE)
	add_executable(uitestapp ${UITestApp_SOURCES}) # hack to enable retina display support
#	add_executable(uitestapp MACOSX_BUNDLE ${UITestApp_SOURCES})
	set(FSLIB fsposix)
elseif(UNIX)
	add_executable(uitestapp ${UITestApp_SOURCES})
	set(FSLIB fsposix)
else()
	message(FATAL_ERROR "Unknown platform")
endif()

target_link_libraries(uitestapp PRIVATE
	${FSLIB}
	platetc
	platglfw
	ui
)
