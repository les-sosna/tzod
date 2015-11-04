cmake_minimum_required (VERSION 3.3)
project(TZodMain CXX)


include_directories(
	${AS_SOURCE_DIR}/inc
	${App_SOURCE_DIR}/inc
	${Config_SOURCE_DIR}/inc
	${Ctx_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${GC_SOURCE_DIR}/inc
	${GV_SOURCE_DIR}/inc
	${Loc_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
	${PlatGLFW_SOURCE_DIR}/inc
	${Render_SOURCE_DIR}/inc
	${Shell_SOURCE_DIR}/inc
	${LibUI_SOURCE_DIR}/inc
	${Video_SOURCE_DIR}/inc

	# 3rd party
	${GLFW_SOURCE_DIR}/include
	${OggVorbis_SOURCE_DIR}/include
)

link_directories(${Boost_LIBRARY_DIR})

find_package(OpenAL)
if(${OPENAL_FOUND})
	include_directories(${Audio_SOURCE_DIR}/inc)
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
else(UNIX)
	add_executable(tzodmain Main.cpp)
else()
	message(FATAL_ERROR "Unknown platform")
endif()

set(tank_LIBRARIES
	platglfw
	video
	shell
	as
	ctx
)

if(${OPENAL_FOUND})
	list(APPEND tank_LIBRARIES audio)
else()
	add_definitions(-DNOSOUND)
endif()

target_link_libraries(tzodmain ${tank_LIBRARIES})
