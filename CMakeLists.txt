cmake_minimum_required(VERSION 3.0.2)
project(Music_Processing)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++11")

include_directories(headers/)

set(SOURCE_FILES source/main.cpp source/audio_decoding.cpp source/fast_fourier_transform.cpp)
set(HEADER_FILES headers/audio_decoding.h)
add_executable(Music_Processing ${HEADER_FILES} ${SOURCE_FILES})

target_link_libraries(Music_Processing vlc fftw3 pthread)