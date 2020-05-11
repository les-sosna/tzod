add_library(loc inc/loc/Language.h Language.cpp)

target_link_libraries(loc PUBLIC config)

target_include_directories(loc INTERFACE inc)
set_target_properties(loc PROPERTIES FOLDER game)