add_library(cbind
	inc/cbind/ConfigBinding.h
	ConfigBinding.cpp
)

target_link_libraries(cbind PRIVATE
	config
	ui
)

target_include_directories(cbind INTERFACE inc)
set_target_properties (cbind PROPERTIES FOLDER engine)
