cmake_minimum_required (VERSION 3.3)
project(App)

if(APPLE)
	set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")
endif()

include_directories(
	${AS_SOURCE_DIR}/inc
	${Config_SOURCE_DIR}/inc
	${Ctx_SOURCE_DIR}/inc
	${FileSystem_SOURCE_DIR}/inc
	${GC_SOURCE_DIR}/inc
	${GV_SOURCE_DIR}/inc
	${Loc_SOURCE_DIR}/inc
	${Math_SOURCE_DIR}/inc
	${Render_SOURCE_DIR}/inc
	${Shell_SOURCE_DIR}/inc
	${LibUI_SOURCE_DIR}/inc
	${Video_SOURCE_DIR}/inc
)

find_package(OpenAL)
if(${OPENAL_FOUND})
	include_directories(${Audio_SOURCE_DIR}/inc)
else()
	add_definitions(-DNOSOUND)
endif()

add_library(app
	tzod.cpp
	View.cpp
)

target_link_libraries(app
	video
	shell
	as
	ctx
)
