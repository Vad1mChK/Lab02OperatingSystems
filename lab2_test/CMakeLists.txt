# Locate Google Test
find_package(GTest REQUIRED)

set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -fsanitize=address -g")
set(CMAKE_LINKER_FLAGS "${CMAKE_LINKER_FLAGS} -fsanitize=address")

# Add lab2_test executable
add_executable(lab2_test
        TestMain.cpp
        Lab2SmokeTests.cpp
)

# Link the lab2_test executable to the library and Google Test
target_link_libraries(lab2_test
        lab2_library
        GTest::GTest
        GTest::Main
)

# Automatically discover tests
include(GoogleTest)
gtest_discover_tests(lab2_test)
