#ifndef LAB2_LIBRARY_HPP
#define LAB2_LIBRARY_HPP
#include <chrono>
#include <string>
#include <sys/types.h>

#define LAB2_DEFAULT_BLOCK_SIZE 4096

typedef int fd_t; // File descriptor

typedef std::chrono::duration<std::nano> access_hint_t;
// access hint is absolute time or a time interval that will help define time for next data access

// Assuming the library will only be used in c++ code

fd_t lab2_open(const std::string& path); // Open file, return handle to file
int lab2_close(fd_t fd);
ssize_t lab2_read(fd_t fd, void* buf, size_t count);
ssize_t lab2_write(fd_t fd, const void* buf, size_t count);
off_t lab2_lseek(fd_t fd, off_t offset, int whence);
int lab2_fsync(fd_t fd);
int lab2_advice(int fd, off_t offset, access_hint_t hint); // Not needed for now

#endif //LAB2_LIBRARY_HPP
