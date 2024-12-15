# tests/CMakeLists.txt

# Locate Google Test
find_package(GTest REQUIRED)

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

# Add the lab2_test executable to CTest
add_test(NAME AllTests COMMAND lab2_test)
