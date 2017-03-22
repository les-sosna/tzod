cmake_minimum_required (VERSION 3.3)
project(GCTests)

add_executable(gc_tests
	PtrList_tests.cpp
)

target_link_libraries(gc_tests
	gc
	gtest_main
)

target_include_directories(gc_tests PRIVATE
	${gtest_SOURCE_DIR}/include
)
