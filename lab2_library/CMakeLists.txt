cmake_minimum_required(VERSION 3.14)
project(useful_library VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Указание исходных файлов
add_library(lab2_library SHARED
        lab2_library.hpp
        lab2_library.cpp
        constants.hpp
        Block.hpp
        Block.cpp
        BlockCache.hpp
        BlockCache.cpp
)
target_include_directories(lab2_library PUBLIC ${CMAKE_CURRENT_SOURCE_DIR})
