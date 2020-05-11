add_library(luaetc LuaDeleter.cpp)

target_link_libraries(luaetc PRIVATE lua)

target_include_directories(luaetc INTERFACE inc)
set_target_properties (luaetc PROPERTIES FOLDER engine)