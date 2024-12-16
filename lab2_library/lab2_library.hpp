#ifndef LAB2_LIBRARY_HPP
#define LAB2_LIBRARY_HPP
#include <string>
#include <sys/types.h>

#include "ClockCache.hpp"
#include "types.hpp"

// Assuming the library will only be used in c++ code, we don't need extern "C" {}
fd_t lab2_open(const std::string &path);
int lab2_close(fd_t fd);
ssize_t lab2_read(fd_t fd, void *buf, size_t count);
ssize_t lab2_write(fd_t fd, const void *buf, size_t count);
off_t lab2_lseek(fd_t fd, off_t offset, int whence);
int lab2_fsync(fd_t fd);

#endif //LAB2_LIBRARY_HPP
