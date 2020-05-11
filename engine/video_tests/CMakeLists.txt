add_executable(video_tests
	RingAllocator_tests.cpp
)

target_link_libraries(video_tests PRIVATE
	video
	gtest_main
)

target_include_directories(video_tests PRIVATE
	${gtest_SOURCE_DIR}/include
)

set_target_properties(video_tests PROPERTIES FOLDER engine)