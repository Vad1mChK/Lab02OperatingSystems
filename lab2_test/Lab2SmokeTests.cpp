#include <gtest/gtest.h>
#include <filesystem>   // for std::filesystem::temp_directory_path, std::filesystem::remove
#include <cstdlib>      // for mkstemp
#include <fcntl.h>      // O_RDWR, etc.
#include <unistd.h>     // close
#include <cstring>      // memset, strlen
#include <string>       // std::string

#include "lab2_library.hpp"

// Helper function to create a unique temporary file path and return it as a std::string.
static std::string makeUniqueTempFile() {
    // Get the system temp directory (e.g., /tmp on Linux)
    std::filesystem::path tempDir = std::filesystem::temp_directory_path();

    // We’ll create a pattern for mkstemp, which replaces "XXXXXX" with random characters
    // Example: /tmp/lab2_test_XXXXXX
    std::filesystem::path templatePath = tempDir / "lab2_test_XXXXXX";
    std::string templateStr = templatePath.string();

    // mkstemp needs a modifiable char array
    std::vector<char> modifiable(templateStr.begin(), templateStr.end());
    modifiable.push_back('\0'); // null-terminate

    // mkstemp replaces "XXXXXX" in-place and creates the file with O_RDWR
    int fd = ::mkstemp(modifiable.data());
    if (fd == -1) {
        throw std::runtime_error("Could not create temp file with mkstemp!");
    }

    // We only want the name; we’ll close the file here
    ::close(fd);

    // Return the actual file path that mkstemp created
    return std::string(modifiable.data());
}

//------------------------------------------------------------------------------
TEST(Lab2SmokeTests, SanityTest) {
#ifndef _5
    // Nod to Penskoy P.
#define _5 (32 / 8)
    ASSERT_EQ(32 / 8, _5);
#undef _5
#else
    ASSERT_NE(32 / 8, 5);
#endif
}

//------------------------------------------------------------------------------
TEST(Lab2Tests, WriteThenRead) {
    // Create a Lab2 instance with capacity=8 blocks, blockSize=4096
    Lab2 lab2(8, 4096);

    // Make unique temporary file
    const std::string tempFile = makeUniqueTempFile();

    // 1) Open file
    fd_t fd = lab2.open(tempFile);
    ASSERT_GE(fd, 0) << "Failed to open temp file for write/read test.";

    // 2) Write some data
    const char *writeMsg = "Hello from Lab2Tests!\n";
    size_t toWrite = std::strlen(writeMsg);
    ssize_t written = lab2.write(fd, writeMsg, toWrite);
    ASSERT_EQ(written, static_cast<ssize_t>(toWrite))
        << "Failed to write the correct number of bytes.";

    // 3) Move offset back to start
    off_t newOffset = lab2.lseek(fd, 0, SEEK_SET);
    ASSERT_EQ(newOffset, 0) << "Failed to lseek back to beginning of file.";

    // 4) Read data
    std::vector<char> buffer(toWrite + 1, '\0');
    ssize_t readBytes = lab2.read(fd, buffer.data(), toWrite);
    ASSERT_EQ(readBytes, static_cast<ssize_t>(toWrite))
        << "Failed to read the correct number of bytes.";

    // 5) Compare
    ASSERT_STREQ(writeMsg, buffer.data())
        << "Read data differs from what was written.";

    // 6) fsync
    int fsyncRes = lab2.fsync(fd);
    ASSERT_EQ(fsyncRes, 0) << "lab2.fsync() returned an error.";

    // 7) close
    int closeRes = lab2.close(fd);
    ASSERT_EQ(closeRes, 0) << "Failed to close the file descriptor.";

    // 8) Cleanup
    std::filesystem::remove(tempFile);
}

//------------------------------------------------------------------------------
TEST(Lab2Tests, LseekBeyondWrite) {
    Lab2 lab2(4, LAB2_BLOCK_SIZE);
    const std::string tempFile = makeUniqueTempFile();

    // 1) Open
    fd_t fd = lab2.open(tempFile);
    ASSERT_GE(fd, 0) << "Failed to open temp file for Lseek test.";

    // 2) Write short data
    const char *shortMsg = "Data\n";
    ssize_t w = lab2.write(fd, shortMsg, std::strlen(shortMsg));
    ASSERT_GT(w, 0) << "Failed to write anything.";

    // 3) Move offset beyond written data
    off_t bigOffset = 1024 * 1024; // e.g., 1 MB into the file
    off_t result = lab2.lseek(fd, bigOffset, SEEK_SET);
    ASSERT_EQ(result, bigOffset);

    // 4) Write more data
    const char *secondMsg = " after the gap\n";
    w = lab2.write(fd, secondMsg, std::strlen(secondMsg));
    ASSERT_EQ(w, static_cast<ssize_t>(std::strlen(secondMsg)));

    // 5) Seek back to start, read entire file
    lab2.lseek(fd, 0, SEEK_SET);

    // We'll read the entire region up to bigOffset + size of secondMsg
    size_t totalSize = bigOffset + std::strlen(secondMsg);
    std::vector<char> buffer(totalSize + 1, '\0');

    ssize_t r = lab2.read(fd, buffer.data(), totalSize);
    ASSERT_EQ(r, static_cast<ssize_t>(totalSize))
        << "Did not read the expected number of bytes.";

    // We expect the buffer to contain "Data\n" + some zeros + " after the gap\n"
    // The offset from 5 to 1MB is presumably zeros (assuming how your code handles unwritten holes).

    // Check the first few bytes match "Data\n"
    ASSERT_TRUE(std::strncmp(buffer.data(), "Data\n", 5) == 0)
        << "Beginning of file doesn't match expected content.";

    // Check the gap is zeroed (this depends on your design; if you do not zero-fill holes,
    // this test may need adjusting).
    for (size_t i = 5; i < bigOffset; ++i) {
        ASSERT_EQ(buffer[i], '\0') << "Gap is not zeroed as expected at offset " << i;
    }

    // Check the trailing part
    ASSERT_TRUE(std::strncmp(buffer.data() + bigOffset, " after the gap\n", 16) == 0)
        << "End of file doesn't match expected content.";

    lab2.close(fd);
    std::filesystem::remove(tempFile);
}

//------------------------------------------------------------------------------
TEST(Lab2Tests, AdviceCallShouldFail) {
    Lab2 lab2(4, LAB2_BLOCK_SIZE);
    const std::string tempFile = makeUniqueTempFile();

    fd_t fd = lab2.open(tempFile);
    ASSERT_GE(fd, 0) << "Failed to open temp file for Advice test.";

    // Since we’re not implementing this in a real sense,
    // we expect it to return -1 or throw.
    int ret = lab2.advice(fd, /*offset=*/0, /*hint=*/0xBADC0FFEE);
    // By assignment: "user can go away" or "return -1" or "throw"
    // If you choose to throw, wrap in try/catch.
    ASSERT_EQ(ret, -1) << "Expected advice() to return -1 for unimplemented method.";

    lab2.close(fd);
    std::filesystem::remove(tempFile);
}
