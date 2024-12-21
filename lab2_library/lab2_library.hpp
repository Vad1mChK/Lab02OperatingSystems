#ifndef LAB2_LIBRARY_HPP
#define LAB2_LIBRARY_HPP
#include <string>
#include <unistd.h>
#include <unordered_map>

#include "BlockCache.hpp"

using fd_t = int;
using access_hint_t = long long;

constexpr size_t LAB2_BLOCK_SIZE = 4096;

class Lab2 {
public:
    explicit Lab2(size_t cacheCapacity, size_t blockSize);

    ~Lab2() = default;

    fd_t open(const std::string &filename);

    int close(fd_t fd);

    ssize_t read(fd_t fd, void *buf, size_t count);

    ssize_t write(fd_t fd, const void *buf, size_t count);

    off_t lseek(fd_t fd, off_t offset, int whence);

    int fsync(fd_t fd);

    static int advice(fd_t fd, off_t offset, access_hint_t hint);

private:
    std::unordered_map<fd_t, off_t> fileOffsets_;
    BlockCache cache_;
};

#endif //LAB2_LIBRARY_HPP
