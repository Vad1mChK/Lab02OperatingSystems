#include "lab2_library.hpp"
#include <fcntl.h>  // For open, O_DIRECT
#include <unistd.h> // For read, write, lseek, fsync
#include <stdexcept>
#include <cstring>

#include "BlockCache.hpp"

static BlockCache cache(64); // Example: Cache with 64 blocks

fd_t lab2_open(const std::string& path) {
    fd_t fd = ::open(path.c_str(), O_RDWR | O_CREAT | O_DIRECT, 0644);
    if (fd < 0) {
        throw std::runtime_error("Failed to open file");
    }
    return fd;
}

int lab2_close(fd_t fd) {
    cache.flush(fd); // Flush dirty blocks
    return ::close(fd);
}

ssize_t lab2_read(fd_t fd, void* buf, size_t count) {
    char* data = cache.read(fd, lseek(fd, 0, SEEK_CUR), count);
    std::memcpy(buf, data, count);
    lseek(fd, count, SEEK_CUR); // Update file pointer
    return count;
}

ssize_t lab2_write(fd_t fd, const void* buf, size_t count) {
    cache.write(fd, lseek(fd, 0, SEEK_CUR), buf, count);
    lseek(fd, count, SEEK_CUR); // Update file pointer
    return count;
}

off_t lab2_lseek(fd_t fd, off_t offset, int whence) {
    return ::lseek(fd, offset, whence);
}

int lab2_fsync(fd_t fd) {
    cache.flush(fd);
    return ::fsync(fd);
}
