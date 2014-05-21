cmake_minimum_required (VERSION 2.8)
project(TankTests)

include_directories(
	${Tank_SOURCE_DIR}
	${gtest_SOURCE_DIR}/include
)

add_executable(tank_tests
	PtrList_tests.cpp
)

target_link_libraries(tank_tests
#	tank
	gtest_main
)
