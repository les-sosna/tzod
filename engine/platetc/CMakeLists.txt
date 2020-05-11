add_library(platetc
	inc/platetc/UIInputRenderingController.h

	UIInputRenderingController.cpp
)

target_link_libraries(platetc
	PRIVATE video
	PUBLIC plat ui
)

target_include_directories(platetc INTERFACE inc)
set_target_properties(platetc PROPERTIES FOLDER engine)