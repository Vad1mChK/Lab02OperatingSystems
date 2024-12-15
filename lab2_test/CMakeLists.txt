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

# Automatically discover tests
include(GoogleTest)
gtest_discover_tests(lab2_test)

# Temporary solution to a temporary problem
# Add tests manually to CTest
add_test(NAME Lab2Library_OpenAndCloseFile COMMAND lab2_test --gtest_filter=Lab2LibrarySmokeTest.OpenAndCloseFile)
add_test(NAME Lab2Library_ReadFile COMMAND lab2_test --gtest_filter=Lab2LibrarySmokeTest.ReadFile)
add_test(NAME Lab2Library_WriteFile COMMAND lab2_test --gtest_filter=Lab2LibrarySmokeTest.WriteFile)
add_test(NAME Lab2Library_SeekAndWriteFile COMMAND lab2_test --gtest_filter=Lab2LibrarySmokeTest.SeekAndWriteFile)
add_test(NAME Lab2Library_FsyncTest COMMAND lab2_test --gtest_filter=Lab2LibrarySmokeTest.FsyncTest)
