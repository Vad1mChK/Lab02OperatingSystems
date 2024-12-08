#include "lab2_library.hpp"
#include <fcntl.h>      // open, O_* flags
#include <unistd.h>     // close, read, write, lseek, fsync
#include <cstdlib>      // posix_memalign
#include <stdexcept>    // std::runtime_error
#include <system_error> // std::system_error
#include <cerrno>       // errno
#include <cstring>

fd_t lab2_open(const std::string& path) {

}

int lab2_close(fd_t fd) {

}

ssize_t lab2_read(fd_t fd, void* buf, size_t count) {

}

ssize_t lab2_write(fd_t fd, const void* buf, size_t count) {

}

off_t lab2_lseek(fd_t fd, off_t offset, int whence) {

}

int lab2_fsync(fd_t fd) {

}