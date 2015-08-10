cmake_minimum_required (VERSION 2.8)
project(GCTests)

include_directories(
	${GC_SOURCE_DIR}/inc
	${gtest_SOURCE_DIR}/include
)

add_executable(gc_tests
	PtrList_tests.cpp
)

target_link_libraries(gc_tests
	gc
	gtest_main
)
