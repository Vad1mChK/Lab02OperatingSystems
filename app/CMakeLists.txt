cmake_minimum_required(VERSION 3.12)
project(app VERSION 1.0 LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 20)
set(CMAKE_CXX_STANDARD_REQUIRED True)

# Указание исходных файлов
add_executable(app app.cpp)

# Указание пути к библиотеке
target_link_libraries(app PRIVATE lab2_library)
target_include_directories(app PRIVATE ../lab2_library)  # Путь к заголовочным файлам