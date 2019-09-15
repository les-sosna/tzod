cmake_minimum_required (VERSION 3.3)

add_library(loc Language.cpp)

target_link_libraries(loc PUBLIC config)

target_include_directories(loc INTERFACE inc)
set_target_properties(loc PROPERTIES FOLDER tzod)