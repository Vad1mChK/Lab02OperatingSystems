cmake_minimum_required(VERSION 3.14)

enable_testing()

project(Lab02OperatingSystems
        VERSION 2024.2.0
        DESCRIPTION "My worst project, I can do better"
        LANGUAGES C CXX
)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

add_subdirectory(app)
add_subdirectory(lab2_library)
add_subdirectory(lab2_test)