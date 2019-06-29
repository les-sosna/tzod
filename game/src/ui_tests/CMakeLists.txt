cmake_minimum_required (VERSION 3.3)
project(UITests)

add_executable(ui_tests
	ScrollView_tests.cpp
	StackLayout_tests.cpp
)

target_link_libraries(ui_tests PRIVATE
	ui
	gtest_main
)

target_include_directories(ui_tests PRIVATE
	${gtest_SOURCE_DIR}/include
)
