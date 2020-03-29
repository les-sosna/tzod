cmake_minimum_required(VERSION 3.4.1)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
set(ANDROID 1)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../.. ${CMAKE_CURRENT_BINARY_DIR}/build)

add_library(tzodjni SHARED
  src/main/cpp/gl_code.cpp
)

# add lib dependencies
target_link_libraries(tzodjni
  app
  fsjni
  fsposix
  platjni
  ui
  video

  android
  log 
  EGL
  GLESv2
)

