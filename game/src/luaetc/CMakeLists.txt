cmake_minimum_required (VERSION 3.3)

add_library(luaetc LuaDeleter.cpp)

target_link_libraries(luaetc PRIVATE lua)

target_include_directories(luaetc INTERFACE inc)
