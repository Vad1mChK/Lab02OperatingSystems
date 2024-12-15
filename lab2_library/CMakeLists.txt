cmake_minimum_required(VERSION 3.12)
project(useful_library VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Указание исходных файлов
add_library(lab2_library SHARED lab2_library.cpp
        Block.hpp
        BlockCache.cpp
        BlockCache.hpp
        ClockEvictor.cpp
        ClockEvictor.hpp
        permissions.hpp
)
target_include_directories(lab2_library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
