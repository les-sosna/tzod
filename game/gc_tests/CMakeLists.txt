add_executable(gc_tests
	Pickup_tests.cpp
	PtrList_tests.cpp
	Serialization_tests.cpp
)

target_link_libraries(gc_tests PRIVATE
	fsmem
	gc
	gtest_main
)

target_include_directories(gc_tests PRIVATE
	${gtest_SOURCE_DIR}/include
)
set_target_properties(gc_tests PROPERTIES FOLDER game)