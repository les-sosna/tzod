cmake_minimum_required (VERSION 3.3)
add_library(pluto pluto.c)
target_link_libraries(pluto lua)
target_include_directories(pluto INTERFACE .)