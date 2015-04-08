cmake_minimum_required (VERSION 2.8)
project(TZodMain CXX)


include_directories(
	${Tank_SOURCE_DIR}
	${FileSystem_SOURCE_DIR}/include
	${GC_SOURCE_DIR}/include
	${Math_SOURCE_DIR}/include
	${UI_SOURCE_DIR}/include
	${Video_SOURCE_DIR}/include

	${OggVorbis_SOURCE_DIR}/include
	${GLFW_SOURCE_DIR}/include
)

link_directories(${Boost_LIBRARY_DIR})

find_package(OpenAL)
if(${OPENAL_FOUND})
else()
	add_definitions(-DNOSOUND)
endif()

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

target_link_libraries(tzodmain tank video)
