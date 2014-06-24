cmake_minimum_required (VERSION 2.8)
project(TZodMain CXX)


include_directories(
	${Tank_SOURCE_DIR}
	${UI_SOURCE_DIR}

	${OggVorbis_SOURCE_DIR}/include
	${GLFW_SOURCE_DIR}/include
)

link_directories(${Boost_LIBRARY_DIR})

if(WIN32)
	add_definitions(-D_WIN32_WINNT=0x0501) # Windows XP
	add_definitions(-D_CRT_SECURE_NO_WARNINGS)
	add_definitions(-DNOMINMAX)
	add_executable(tzodmain WIN32 Main.cpp)
elseif(APPLE)
	add_executable(tzodmain MACOSX_BUNDLE Main.cpp)
else()
	message(FATAL_ERROR "Unknown platform")
endif()

target_link_libraries(tzodmain tank)
