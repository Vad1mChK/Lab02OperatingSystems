cmake_minimum_required(VERSION 3.12)

project(Lab02OperatingSystems
        VERSION 2024.2.0
        DESCRIPTION "My worst project"
        LANGUAGES C CXX
)

set(CMAKE_C_STANDARD 23)
set(CMAKE_C_STANDARD_REQUIRED True)

set(CMAKE_CXX_STANDARD 23)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(app)
