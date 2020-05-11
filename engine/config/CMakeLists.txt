add_library(config
	inc/config/ConfigBase.h
	inc/config/ConfigCache.h

	ConfigBase.cpp
	ConfigCache.cpp
)

target_link_libraries(config PRIVATE
	fs
	luaetc
	lua #external
)

target_include_directories(config INTERFACE inc)
set_target_properties (config PROPERTIES FOLDER engine)