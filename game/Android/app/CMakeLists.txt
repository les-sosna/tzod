cmake_minimum_required(VERSION 3.4.1)

# now build app's shared lib
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++1z")
set(ANDROID 1)

add_subdirectory(${CMAKE_CURRENT_SOURCE_DIR}/../../src ${CMAKE_CURRENT_BINARY_DIR}/game/src)

add_library(tzodjni SHARED
            src/main/cpp/gl_code.cpp
            src/main/cpp/JniAppWindow.cpp
            src/main/cpp/JniAppWindow.h
            src/main/cpp/JniClipboard.cpp
            src/main/cpp/JniClipboard.h
            src/main/cpp/JniConsoleLog.cpp
            src/main/cpp/JniConsoleLog.h
            src/main/cpp/JniInput.cpp
            src/main/cpp/JniInput.h
            )

# add lib dependencies
target_link_libraries(tzodjni

                      app
                      fs
                      plat
                      ui
                      video

                      android
                      log 
                      EGL
                      GLESv2)

