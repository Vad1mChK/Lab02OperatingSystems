#include "lab2_library.hpp"
#include <fcntl.h>
#include <unistd.h>
#include <cstring>
#include <stdexcept>
#include <iostream>
#include <sys/stat.h>
#include <sys/types.h>

#include "BlockCache.hpp"

struct  Lab2::BlockCacheWrapper {
    BlockCache cache_;

    BlockCacheWrapper(size_t cacheCapacity, size_t blockSize): cache_(cacheCapacity, blockSize) {}
};

Lab2::Lab2(size_t cacheCapacity, size_t blockSize):
    fileOffsets_(),
    cacheWrapper_(std::make_unique<BlockCacheWrapper>(cacheCapacity, blockSize)) {
    // Any additional initialization
}

Lab2::~Lab2() = default; // Defined in the .cpp file

fd_t Lab2::open(const std::string &filename) {
    // Open with O_DIRECT to bypass page cache
    // (the file must be aligned for reads/writes).
    // Using O_RDWR for both read & write in this example:

    // Access rights for file if creating a new file. E.g. 0644 = rw-r--r--
    constexpr int ACCESS_RIGHTS = S_IRUSR | S_IWUSR | S_IRGRP | S_IROTH;
    int realFd = ::open(filename.c_str(), O_RDWR | O_DIRECT | O_SYNC | O_CREAT, ACCESS_RIGHTS);
    if (realFd < 0) {
        // In production code, handle errors properly (set errno, throw, etc.)
        std::cerr << "Failed to open file: " << filename << "\n";
        return -1;
    }

    // Initialize the file offset to 0
    fileOffsets_[realFd] = 0;
    return realFd; // Return the same as "fake fd" for simplicity
}

int Lab2::close(fd_t fd) {
    auto it = fileOffsets_.find(fd);
    if (it == fileOffsets_.end()) {
        // No such FD
        errno = EBADF;
        return -1;
    }

    // Make sure to flush blocks belonging to this fd
    fsync(fd);

    fileOffsets_.erase(it);
    return ::close(fd);
}

ssize_t Lab2::read(fd_t fd, void *buf, size_t count) {
    auto it = fileOffsets_.find(fd);
    if (it == fileOffsets_.end()) {
        errno = EBADF;
        return -1;
    }

    off_t &offset = it->second; // Current offset in the file
    size_t bytesRead = 0;
    char *outPtr = static_cast<char *>(buf);

    while (bytesRead < count) {
        // Calculate which block we need to read
        off_t blockIndex = offset / cacheWrapper_->cache_.blockSize();
        size_t offsetInBlock = offset % cacheWrapper_->cache_.blockSize();

        // Amount we can read from this block
        size_t canRead = cacheWrapper_->cache_.blockSize() - offsetInBlock;
        size_t left = count - bytesRead;
        size_t toReadNow = (left < canRead) ? left : canRead;

        // Fetch data from the cache (which might load from disk)
        bool success = cacheWrapper_->cache_.readBlock(fd, blockIndex);
        if (!success) {
            // Error reading the block from disk
            return -1;
        }
        // Copy from cache to user buffer
        const char *blockData = static_cast<const char *>(cacheWrapper_->cache_.blockData(fd, blockIndex));
        if (!blockData) {
            // Something is wrong, e.g. block not found
            return -1;
        }

        std::memcpy(outPtr + bytesRead,
                    blockData + offsetInBlock,
                    toReadNow);

        // Update counters
        bytesRead += toReadNow;
        offset += toReadNow;
    }

    return bytesRead;
}

ssize_t Lab2::write(fd_t fd, const void *buf, size_t count) {
    auto it = fileOffsets_.find(fd);
    if (it == fileOffsets_.end()) {
        errno = EBADF;
        return -1;
    }

    off_t &offset = it->second;
    size_t bytesWritten = 0;
    const char *inPtr = static_cast<const char *>(buf);

    while (bytesWritten < count) {
        off_t blockIndex = offset / cacheWrapper_->cache_.blockSize();
        size_t offsetInBlock = offset % cacheWrapper_->cache_.blockSize();

        size_t canWrite = cacheWrapper_->cache_.blockSize() - offsetInBlock;
        size_t left = count - bytesWritten;
        size_t toWriteNow = (left < canWrite) ? left : canWrite;

        // Fetch the block in case we need partial update, or we do read-modify-write
        if (!cacheWrapper_->cache_.readBlock(fd, blockIndex)) {
            // Could not load block
            return -1;
        }
        // Mark block as "dirty" after we copy data in
        char *blockData = static_cast<char *>(cacheWrapper_->cache_.blockData(fd, blockIndex));
        if (!blockData) {
            // Something is wrong
            return -1;
        }

        // Copy user data to cache
        std::memcpy(blockData + offsetInBlock,
                    inPtr + bytesWritten,
                    toWriteNow);

        cacheWrapper_->cache_.markDirty(fd, blockIndex); // Mark as dirty in the cache

        bytesWritten += toWriteNow;
        offset += toWriteNow;
    }

    return bytesWritten;
}

off_t Lab2::lseek(fd_t fd, off_t offset, int whence) {
    auto it = fileOffsets_.find(fd);
    if (it == fileOffsets_.end()) {
        errno = EBADF;
        return static_cast<off_t>(-1);
    }

    off_t newOffset = 0;

    if (whence == SEEK_SET) {
        newOffset = offset;
    } else if (whence == SEEK_CUR) {
        newOffset = it->second + offset;
    } else if (whence == SEEK_END) {
        // We need to know the file's actual size
        // Temporarily get the OS's idea of file size
        off_t fileSize = ::lseek(fd, 0, SEEK_END);
        if (fileSize == static_cast<off_t>(-1)) {
            // Error
            return static_cast<off_t>(-1);
        }
        newOffset = fileSize + offset;
    } else {
        errno = EINVAL;
        return static_cast<off_t>(-1);
    }

    if (newOffset < 0) {
        errno = EINVAL;
        return static_cast<off_t>(-1);
    }

    it->second = newOffset;
    return newOffset;
}

int Lab2::fsync(fd_t fd) {
    // 1) Flush dirty blocks for this fd in the cache
    cacheWrapper_->cache_.flushFd(fd);

    // 2) Then call the OS fsync
    if (::fsync(fd) < 0) {
        return -1;
    }
    return 0;
}

int Lab2::advice(fd_t fd, off_t offset, access_hint_t hint) {
    // By assignment: not implemented or “tell the user to go away”
    // We can just return -1 or throw
    std::cerr << "advice(fd_t, off_t, access_hint_t) not implemented, ignoring hint = " << hint << "\n";
    return -1;
}
