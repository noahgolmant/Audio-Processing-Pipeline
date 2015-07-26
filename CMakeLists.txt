cmake_minimum_required(VERSION 3.0.2)
project(Music_Processing)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

set(SOURCE_FILES main.cpp audio_objects.c)
add_executable(Music_Processing ${SOURCE_FILES})

target_link_libraries(Music_Processing vlc)