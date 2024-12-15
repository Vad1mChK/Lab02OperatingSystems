//
// Created by vad1mchk on 15/12/24.
//
#include <gtest/gtest.h>
#include "../lab2_library/lab2_library.hpp"
#include <cstring>
#include <fstream>

// Helper function to create a test file
void create_test_file(const std::string& path, const std::string& content) {
    std::ofstream file(path, std::ios::binary);
    file << content;
    file.close();
}

TEST(Lab2LibrarySmokeTest, OpenAndCloseFile) {
    const std::string test_file = "testfile_open.txt";
    create_test_file(test_file, "Test data for open and close");

    fd_t fd = lab2_open(test_file);
    ASSERT_GT(fd, 0) << "Failed to open file";

    int result = lab2_close(fd);
    ASSERT_EQ(result, 0) << "Failed to close file";

    // Clean up
    std::remove(test_file.c_str());
}

TEST(Lab2LibrarySmokeTest, ReadFile) {
    const std::string test_file = "testfile_read.txt";
    const std::string content = "Hello, World!";
    create_test_file(test_file, content);

    fd_t fd = lab2_open(test_file);
    ASSERT_GT(fd, 0) << "Failed to open file for reading";

    char buffer[64] = {0};
    ssize_t bytes_read = lab2_read(fd, buffer, content.size());
    ASSERT_EQ(bytes_read, content.size()) << "Read wrong number of bytes";
    ASSERT_STREQ(buffer, content.c_str()) << "Read content mismatch";

    lab2_close(fd);

    // Clean up
    std::remove(test_file.c_str());
}

TEST(Lab2LibrarySmokeTest, WriteFile) {
    const std::string test_file = "testfile_write.txt";
    const std::string content = "Data to be written";

    fd_t fd = lab2_open(test_file);
    ASSERT_GT(fd, 0) << "Failed to open file for writing";

    ssize_t bytes_written = lab2_write(fd, content.c_str(), content.size());
    ASSERT_EQ(bytes_written, content.size()) << "Failed to write all bytes";

    lab2_close(fd);

    // Verify file content
    std::ifstream file(test_file, std::ios::binary);
    std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    ASSERT_EQ(file_content, content) << "File content mismatch after write";

    // Clean up
    std::remove(test_file.c_str());
}

TEST(Lab2LibrarySmokeTest, SeekAndWriteFile) {
    const std::string test_file = "testfile_seek.txt";
    create_test_file(test_file, "Initial content...");

    fd_t fd = lab2_open(test_file);
    ASSERT_GT(fd, 0) << "Failed to open file for seeking";

    off_t new_position = lab2_lseek(fd, 8, SEEK_SET);
    ASSERT_EQ(new_position, 8) << "Failed to seek to position 8";

    const std::string content = "UPDATED";
    ssize_t bytes_written = lab2_write(fd, content.c_str(), content.size());
    ASSERT_EQ(bytes_written, content.size()) << "Failed to write all bytes after seek";

    lab2_close(fd);

    // Verify file content
    std::ifstream file(test_file, std::ios::binary);
    std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    ASSERT_EQ(file_content, "Initial UPDATED...") << "File content mismatch after seek and write";

    // Clean up
    std::remove(test_file.c_str());
}

TEST(Lab2LibrarySmokeTest, FsyncTest) {
    const std::string test_file = "testfile_fsync.txt";
    create_test_file(test_file, "Initial fsync test content");

    fd_t fd = lab2_open(test_file);
    ASSERT_GT(fd, 0) << "Failed to open file for fsync test";

    const std::string content = "FSYNC UPDATED";
    ssize_t bytes_written = lab2_write(fd, content.c_str(), content.size());
    ASSERT_EQ(bytes_written, content.size()) << "Failed to write all bytes for fsync test";

    int sync_result = lab2_fsync(fd);
    ASSERT_EQ(sync_result, 0) << "Failed to fsync the file";

    lab2_close(fd);

    // Verify file content
    std::ifstream file(test_file, std::ios::binary);
    std::string file_content((std::istreambuf_iterator<char>(file)), std::istreambuf_iterator<char>());
    ASSERT_EQ(file_content, content) << "File content mismatch after fsync";

    // Clean up
    std::remove(test_file.c_str());
}
